ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_trig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:56 $
                                    
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
define(FILE_trig)


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
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_trig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:56 $
                                    
#
#	ssin.sa 3.3 7/29/91
#
#	The entry point sSIN computes the sine of an input argument
#	sCOS computes the cosine, and sSINCOS computes both. The
#	corresponding entry points with a "d" computes the same
#	corresponding function values for denormalized inputs.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The funtion value sin(X) or cos(X) returned in Fp0 if SIN or
#		COS is requested. Otherwise, for SINCOS, sin(X) is returned
#		in Fp0, and cos(X) is returned in Fp1.
#
#	Modifies: Fp0 for SIN or COS; both Fp0 and Fp1 for SINCOS.
#
#	Accuracy and Monotonicity: The returned result is within 1 ulp in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The programs sSIN and sCOS take approximately 150 cycles for
#		input argument X such that |X| < 15Pi, which is the the usual
#		situation. The speed for sSINCOS is approximately 190 cycles.
#
#	Algorithm:
#
#	SIN and COS:
#	1. If SIN is invoked, set ssins_AdjN := 0; otherwise, set ssins_AdjN := 1.
#
#	2. If |X| >= 15Pi or |X| < 2**(-40), go to 7.
#
#	3. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 4, so in particular, k = 0,1,2,or 3. Overwirte
#		k by k := k + ssins_AdjN.
#
#	4. If k is even, go to 6.
#
#	5. (k is odd) Set j := (k-1)/2, sgn := (-1)**j. Return sgn*cos(r)
#		where cos(r) is approximated by an even polynomial in r,
#		1 + r*r*(B1+s*(B2+ ... + s*B8)),	s = r*r.
#		Exit.
#
#	6. (k is even) Set j := k/2, sgn := (-1)**j. Return sgn*sin(r)
#		where sin(r) is approximated by an odd polynomial in r
#		r + r*s*(A1+s*(A2+ ... + s*A7)),	s = r*r.
#		Exit.
#
#	7. If |X| > 1, go to 9.
#
#	8. (|X|<2**(-40)) If SIN is invoked, return X; otherwise return 1.
#
#	9. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 3.
#
#	SINCOS:
#	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.
#
#	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 4, so in particular, k = 0,1,2,or 3.
#
#	3. If k is even, go to 5.
#
#	4. (k is odd) Set j1 := (k-1)/2, j2 := j1 (EOR) (k mod 2), i.e.
#		j1 exclusive or with the l.s.b. of k.
#		sgn1 := (-1)**j1, sgn2 := (-1)**j2.
#		SIN(X) = sgn1 * cos(r) and COS(X) = sgn2*sin(r) where
#		sin(r) and cos(r) are computed as odd and even polynomials
#		in r, respectively. Exit
#
#	5. (k is even) Set j1 := k/2, sgn1 := (-1)**j1.
#		SIN(X) = sgn1 * sin(r) and COS(X) = sgn1*cos(r) where
#		sin(r) and cos(r) are computed as odd and even polynomials
#		in r, respectively. Exit
#
#	6. If |X| > 1, go to 8.
#
#	7. (|X|<2**(-40)) SIN(X) = X and COS(X) = 1. Exit.
#
#	8. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 2.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
ssins_bounds1: long      0x3fd78000,0x4004bc7e 
ssins_twobypi: long      0x3fe45f30,0x6dc9c883 
                                    
ssins_sina7:   long      0xbd6aaa77,0xccc994f5 
ssins_sina6:   long      0x3de61209,0x7aae8da1 
                                    
ssins_sina5:   long      0xbe5ae645,0x2a118ae4 
ssins_sina4:   long      0x3ec71de3,0xa5341531 
                                    
ssins_sina3:   long      0xbf2a01a0,0x1a018b59,0x00000000,0x00000000 
                                    
ssins_sina2:   long      0x3ff80000,0x88888888,0x888859af,0x00000000 
                                    
ssins_sina1:   long      0xbffc0000,0xaaaaaaaa,0xaaaaaa99,0x00000000 
                                    
ssins_cosb8:   long      0x3d2ac4d0,0xd6011ee3 
ssins_cosb7:   long      0xbda9396f,0x9f45ac19 
                                    
ssins_cosb6:   long      0x3e21eed9,0x0612c972 
ssins_cosb5:   long      0xbe927e4f,0xb79d9fcf 
                                    
ssins_cosb4:   long      0x3efa01a0,0x1a01d423,0x00000000,0x00000000 
                                    
ssins_cosb3:   long      0xbff50000,0xb60b60b6,0x0b61d438,0x00000000 
                                    
ssins_cosb2:   long      0x3ffa0000,0xaaaaaaaa,0xaaaaab5e 
ssins_cosb1:   long      0xbf000000       
                                    
ssins_invtwopi: long      0x3ffc0000,0xa2f9836e,0x4e44152a 
                                    
ssins_twopi1:  long      0x40010000,0xc90fdaa2,0x00000000,0x00000000 
ssins_twopi2:  long      0x3fdf0000,0x85a308d4,0x00000000,0x00000000 
                                    
                                    
                                    
         set       ssins_inarg,fp_scr4    
                                    
         set       ssins_x,fp_scr5        
         set       ssins_xdcare,ssins_x+2       
         set       ssins_xfrac,ssins_x+4        
                                    
         set       ssins_rprime,fp_scr1   
         set       ssins_sprime,fp_scr2   
                                    
         set       ssins_posneg1,l_scr1   
         set       ssins_twoto63,l_scr1   
                                    
         set       ssins_endflag,l_scr2   
         set       ssins_n,l_scr2         
                                    
         set       ssins_adjn,l_scr3      
                                    
                                    
                                    
                                    
                                    
ssind:                              
#--SIN(X) = X FOR DENORMALIZED X
         bra.l     t_extdnrm        
                                    
scosd:                              
#--COS(X) = 1 FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp0      
#
#	9D25B Fix: Sometimes the previous fmove.s sets fpsr bits
#
         fmove.l   &0,%fpsr         
#
         bra.l     t_frcinx         
                                    
ssin:                               
#--SET ADJN TO 0
         move.l    &0,ssins_adjn(%a6)     
         bra.b     ssins_sinbgn           
                                    
scos:                               
#--SET ADJN TO 1
         move.l    &1,ssins_adjn(%a6)     
                                    
ssins_sinbgn:                             
#--SAVE FPCR, FP1. CHECK IF |X| IS TOO SMALL OR LARGE
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,ssins_x(%a6)      
         andi.l    &0x7fffffff,%d0  # COMPACTIFY X
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     ssins_sok1             
         bra.w     ssins_sinsm            
                                    
ssins_sok1:                               
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     ssins_sinmain          
         bra.w     ssins_reducex          
                                    
ssins_sinmain:                            
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
ifdef(`PIC',`	fmul.d	&0x3fe45f306dc9c883,%fp1
',`         fmul.d    ssins_twobypi,%fp1     # X*2/PI
')
                                    
#--HIDE THE NEXT THREE INSTRUCTIONS
ifdef(`PIC',`	lea	pitbl+0x200(%pc),%a1
',`         lea       pitbl+0x200,%a1  # ,32
')
                                    
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,ssins_n(%a6)      # CONVERT TO INTEGER
                                    
         move.l    ssins_n(%a6),%d0       
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # A1 IS THE ADDRESS OF N*PIBY2
#				...WHICH IS IN TWO PIECES Y1 & Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
#--HIDE THE NEXT ONE
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
ssins_sincont:                            
#--continuation from REDUCEX
                                    
#--GET N+ADJN AND SEE IF SIN(R) OR COS(R) IS NEEDED
         move.l    ssins_n(%a6),%d0       
         add.l     ssins_adjn(%a6),%d0    # SEE IF D0 IS ODD OR EVEN
         ror.l     &1,%d0           # D0 WAS ODD IFF D0 IS NEGATIVE
         cmpi.l    %d0,&0           
         blt.w     ssins_cospoly          
                                    
ssins_sinpoly:                            
#--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
#--THEN WE RETURN	SGN*SIN(R). SGN*SIN(R) IS COMPUTED BY
#--R' + R'*S*(A1 + S(A2 + S(A3 + S(A4 + ... + SA7)))), WHERE
#--R' = SGN*R, S=R*R. THIS CAN BE REWRITTEN AS
#--R' + R'*S*( [A1+T(A3+T(A5+TA7))] + [S(A2+T(A4+TA6))])
#--WHERE T=S*S.
#--NOTE THAT A3 THROUGH A7 ARE STORED IN DOUBLE PRECISION
#--WHILE A1 AND A2 ARE IN DOUBLE-EXTENDED FORMAT.
         fmove.x   %fp0,ssins_x(%a6)      # X IS R
         fmul.x    %fp0,%fp0        # FP0 IS S
#---HIDE THE NEXT TWO WHILE WAITING FOR FP0
ifdef(`PIC',`	fmove.d	&0xbd6aaa77ccc994f5,%fp3
',`         fmove.d   ssins_sina7,%fp3       
')
ifdef(`PIC',`	fmove.d	&0x3de612097aae8da1,%fp2
',`         fmove.d   ssins_sina6,%fp2       
')
#--FP0 IS NOW READY
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS T
#--HIDE THE NEXT TWO WHILE WAITING FOR FP1
                                    
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
#				...LEAST SIG. BIT OF D0 IN SIGN POSITION
         eor.l     %d0,ssins_x(%a6)       # X IS NOW R'= SGN*R
                                    
         fmul.x    %fp1,%fp3        # TA7
         fmul.x    %fp1,%fp2        # TA6
                                    
ifdef(`PIC',`	fadd.d	&0xbe5ae6452a118ae4,%fp3
',`         fadd.d    ssins_sina5,%fp3       # A5+TA7
')
ifdef(`PIC',`	fadd.d	&0x3ec71de3a5341531,%fp2
',`         fadd.d    ssins_sina4,%fp2       # A4+TA6
')
                                    
         fmul.x    %fp1,%fp3        # T(A5+TA7)
         fmul.x    %fp1,%fp2        # T(A4+TA6)
                                    
ifdef(`PIC',`	fadd.d	&0xbf2a01a01a018b59,%fp3
',`         fadd.d    ssins_sina3,%fp3       # A3+T(A5+TA7)
')
ifdef(`PIC',`	fadd.x	&0x3ff8000088888888888859af,%fp2
',`         fadd.x    ssins_sina2,%fp2       # A2+T(A4+TA6)
')
                                    
         fmul.x    %fp3,%fp1        # T(A3+T(A5+TA7))
                                    
         fmul.x    %fp0,%fp2        # S(A2+T(A4+TA6))
ifdef(`PIC',`	fadd.x	&0xbffc0000aaaaaaaaaaaaaa99,%fp1
',`         fadd.x    ssins_sina1,%fp1       # A1+T(A3+T(A5+TA7))
')
         fmul.x    ssins_x(%a6),%fp0      # R'*S
                                    
         fadd.x    %fp2,%fp1        # [A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))]
#--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
#--FP2 RELEASED, RESTORE NOW AND TAKE FULL ADVANTAGE OF HIDING
                                    
                                    
         fmul.x    %fp1,%fp0        # SIN(R__hpasm_string1
#--FP1 RELEASED.
                                    
         fmove.l   %d1,%fpcr        # ssins_restore users exceptions
         fadd.x    ssins_x(%a6),%fp0      # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
ssins_cospoly:                            
#--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
#--THEN WE RETURN	SGN*COS(R). SGN*COS(R) IS COMPUTED BY
#--SGN + S'*(B1 + S(B2 + S(B3 + S(B4 + ... + SB8)))), WHERE
#--S=R*R AND S'=SGN*S. THIS CAN BE REWRITTEN AS
#--SGN + S'*([B1+T(B3+T(B5+TB7))] + [S(B2+T(B4+T(B6+TB8)))])
#--WHERE T=S*S.
#--NOTE THAT B4 THROUGH B8 ARE STORED IN DOUBLE PRECISION
#--WHILE B2 AND B3 ARE IN DOUBLE-EXTENDED FORMAT, B1 IS -1/2
#--AND IS THEREFORE STORED AS SINGLE PRECISION.
                                    
         fmul.x    %fp0,%fp0        # FP0 IS S
#---HIDE THE NEXT TWO WHILE WAITING FOR FP0
ifdef(`PIC',`	fmove.d	&0x3d2ac4d0d6011ee3,%fp2
',`         fmove.d   ssins_cosb8,%fp2       
')
ifdef(`PIC',`	fmove.d	&0xbda9396f9f45ac19,%fp3
',`         fmove.d   ssins_cosb7,%fp3       
')
#--FP0 IS NOW READY
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS T
#--HIDE THE NEXT TWO WHILE WAITING FOR FP1
         fmove.x   %fp0,ssins_x(%a6)      # X IS S
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
#			...LEAST SIG. BIT OF D0 IN SIGN POSITION
                                    
         fmul.x    %fp1,%fp2        # TB8
#--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
         eor.l     %d0,ssins_x(%a6)       # X IS NOW S'= SGN*S
         andi.l    &0x80000000,%d0  
                                    
         fmul.x    %fp1,%fp3        # TB7
#--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
         ori.l     &0x3f800000,%d0  # D0 IS SGN IN SINGLE
         move.l    %d0,ssins_posneg1(%a6) 
                                    
ifdef(`PIC',`	fadd.d	&0x3e21eed90612c972,%fp2
',`         fadd.d    ssins_cosb6,%fp2       # B6+TB8
')
ifdef(`PIC',`	fadd.d	&0xbe927e4fb79d9fcf,%fp3
',`         fadd.d    ssins_cosb5,%fp3       # B5+TB7
')
                                    
         fmul.x    %fp1,%fp2        # T(B6+TB8)
         fmul.x    %fp1,%fp3        # T(B5+TB7)
                                    
ifdef(`PIC',`	fadd.d	&0x3efa01a01a01d423,%fp2
',`         fadd.d    ssins_cosb4,%fp2       # B4+T(B6+TB8)
')
ifdef(`PIC',`	fadd.x	&0xbff50000b60b60b60b61d438,%fp3
',`         fadd.x    ssins_cosb3,%fp3       # B3+T(B5+TB7)
')
                                    
         fmul.x    %fp1,%fp2        # T(B4+T(B6+TB8))
         fmul.x    %fp3,%fp1        # T(B3+T(B5+TB7))
                                    
ifdef(`PIC',`	fadd.x	&0x3ffa0000aaaaaaaaaaaaab5e,%fp2
',`         fadd.x    ssins_cosb2,%fp2       # B2+T(B4+T(B6+TB8))
')
ifdef(`PIC',`	fadd.s	&0xbf000000,%fp1
',`         fadd.s    ssins_cosb1,%fp1       # B1+T(B3+T(B5+TB7))
')
                                    
         fmul.x    %fp2,%fp0        # S(B2+T(B4+T(B6+TB8)))
#--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
#--FP2 RELEASED.
                                    
                                    
         fadd.x    %fp1,%fp0        
#--FP1 RELEASED
                                    
         fmul.x    ssins_x(%a6),%fp0      
                                    
         fmove.l   %d1,%fpcr        # ssins_restore users exceptions
         fadd.s    ssins_posneg1(%a6),%fp0 # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
ssins_sinbors:                            
#--IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION.
#--IF |X| < 2**(-40), RETURN X OR 1.
         cmpi.l    %d0,&0x3fff8000  
         bgt.b     ssins_reducex          
                                    
                                    
ssins_sinsm:                              
         move.l    ssins_adjn(%a6),%d0    
         cmpi.l    %d0,&0           
         bgt.b     ssins_costiny          
                                    
ssins_sintiny:                            
         move.w    &0x0000,ssins_xdcare(%a6) # JUST IN CASE
         fmove.l   %d1,%fpcr        # ssins_restore users exceptions
         fmove.x   ssins_x(%a6),%fp0      # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
ssins_costiny:                            
         fmove.s   &0f1.0,%fp0      
                                    
         fmove.l   %d1,%fpcr        # ssins_restore users exceptions
         fsub.s    &0f1.1754943e-38,%fp0 # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
ssins_reducex:                            
#--WHEN REDUCEX IS USED, THE CODE WILL INEVITABLY BE SLOW.
#--THIS REDUCTION METHOD, HOWEVER, IS MUCH FASTER THAN USING
#--THE REMAINDER INSTRUCTION WHICH IS NOW IN SOFTWARE.
                                    
         fmovem.x  %fp2-%fp5,-(%a7) # save FP2 through FP5
         move.l    %d2,-(%a7)       
         fmove.s   &0f0.0,%fp1      
#--If compact form of abs(arg) in d0=$7ffeffff, argument is so large that
#--there is a danger of unwanted overflow in first LOOP iteration.  In this
#--case, reduce argument by one remainder step to make subsequent reduction
#--safe.
         cmpi.l    %d0,&0x7ffeffff  # is argument dangerously large?
         bne.b     ssins_loop             
         move.l    &0x7ffe0000,fp_scr2(%a6) # yes
#					;create 2**16383*PI/2
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   
         ftest.x   %fp0             # test sign of argument
         move.l    &0x7fdc0000,fp_scr3(%a6) # create low half of 2**16383*
#					;PI/2 at FP_SCR3
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   
         fblt.w    ssins_red_neg          
         or.w      &0x8000,fp_scr2(%a6) # positive arg
         or.w      &0x8000,fp_scr3(%a6) 
ssins_red_neg:                            
         fadd.x    fp_scr2(%a6),%fp0 # high part of reduction is exact
         fmove.x   %fp0,%fp1        # save high result in fp1
         fadd.x    fp_scr3(%a6),%fp0 # low part of reduction
         fsub.x    %fp0,%fp1        # determine low component of result
         fadd.x    fp_scr3(%a6),%fp1 # fp0/fp1 are reduced argument.
                                    
#--ON ENTRY, FP0 IS X, ON RETURN, FP0 IS X REM PI/2, |X| <= PI/4.
#--integer quotient will be stored in N
#--Intermeditate remainder is 66-bit long; (R,r) in (FP0,FP1)
                                    
ssins_loop:                               
         fmove.x   %fp0,ssins_inarg(%a6)  # +-2**K * F, 1 <= F < 2
         move.w    ssins_inarg(%a6),%d0   
         move.l    %d0,%a1          # save a copy of D0
         andi.l    &0x00007fff,%d0  
         subi.l    &0x00003fff,%d0  # D0 IS K
         cmpi.l    %d0,&28          
         ble.b     ssins_lastloop         
ssins_contloop:                            
         subi.l    &27,%d0          # D0 IS L := K-27
         move.l    &0,ssins_endflag(%a6)  
         bra.b     ssins_work             
ssins_lastloop:                            
         clr.l     %d0              # D0 IS L := 0
         move.l    &1,ssins_endflag(%a6)  
                                    
ssins_work:                               
#--FIND THE REMAINDER OF (R,r) W.R.T.	2**L * (PI/2). L IS SO CHOSEN
#--THAT	INT( X * (2/PI) / 2**(L) ) < 2**29.
                                    
#--CREATE 2**(-L) * (2/PI), SIGN(INARG)*2**(63),
#--2**L * (PIby2_1), 2**L * (PIby2_2)
                                    
         move.l    &0x00003ffe,%d2  # BIASED EXPO OF 2/PI
         sub.l     %d0,%d2          # BIASED EXPO OF 2**(-L)*(2/PI)
                                    
         move.l    &0xa2f9836e,fp_scr1+4(%a6) 
         move.l    &0x4e44152a,fp_scr1+8(%a6) 
         move.w    %d2,fp_scr1(%a6) # FP_SCR1 is 2**(-L)*(2/PI)
                                    
         fmove.x   %fp0,%fp2        
         fmul.x    fp_scr1(%a6),%fp2 
#--WE MUST NOW FIND INT(FP2). SINCE WE NEED THIS VALUE IN
#--FLOATING POINT FORMAT, THE TWO FMOVE'S	FMOVE.L FP <--> N
#--WILL BE TOO INEFFICIENT. THE WAY AROUND IT IS THAT
#--(SIGN(INARG)*2**63	+	FP2) - SIGN(INARG)*2**63 WILL GIVE
#--US THE DESIRED VALUE IN FLOATING POINT.
                                    
#--HIDE SIX CYCLES OF INSTRUCTION
         move.l    %a1,%d2          
         swap      %d2              
         andi.l    &0x80000000,%d2  
         ori.l     &0x5f000000,%d2  # D2 IS SIGN(INARG)*2**63 IN SGL
         move.l    %d2,ssins_twoto63(%a6) 
                                    
         move.l    %d0,%d2          
         addi.l    &0x00003fff,%d2  # BIASED EXPO OF 2**L * (PI/2)
                                    
#--FP2 IS READY
         fadd.s    ssins_twoto63(%a6),%fp2 # THE FRACTIONAL PART OF FP1 IS ROUNDED
                                    
#--HIDE 4 CYCLES OF INSTRUCTION; creating 2**(L)*Piby2_1  and  2**(L)*Piby2_2
         move.w    %d2,fp_scr2(%a6) 
         clr.w     fp_scr2+2(%a6)   
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   # FP_SCR2 is  2**(L) * Piby2_1	
                                    
#--FP2 IS READY
         fsub.s    ssins_twoto63(%a6),%fp2 # FP2 is N
                                    
         addi.l    &0x00003fdd,%d0  
         move.w    %d0,fp_scr3(%a6) 
         clr.w     fp_scr3+2(%a6)   
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   # FP_SCR3 is 2**(L) * Piby2_2
                                    
         move.l    ssins_endflag(%a6),%d0 
                                    
#--We are now ready to perform (R+r) - N*P1 - N*P2, P1 = 2**(L) * Piby2_1 and
#--P2 = 2**(L) * Piby2_2
         fmove.x   %fp2,%fp4        
         fmul.x    fp_scr2(%a6),%fp4 # W = N*P1
         fmove.x   %fp2,%fp5        
         fmul.x    fp_scr3(%a6),%fp5 # w = N*P2
         fmove.x   %fp4,%fp3        
#--we want P+p = W+w  but  |p| <= half ulp of P
#--Then, we need to compute  A := R-P   and  a := r-p
         fadd.x    %fp5,%fp3        # FP3 is P
         fsub.x    %fp3,%fp4        # W-P
                                    
         fsub.x    %fp3,%fp0        # FP0 is A := R - P
         fadd.x    %fp5,%fp4        # FP4 is p = (W-P)+w
                                    
         fmove.x   %fp0,%fp3        # FP3 A
         fsub.x    %fp4,%fp1        # FP1 is a := r - p
                                    
#--Now we need to normalize (A,a) to  "new (R,r)" where R+r = A+a but
#--|r| <= half ulp of R.
         fadd.x    %fp1,%fp0        # FP0 is R := A+a
#--No need to calculate r if this is the last ssins_loop
         cmpi.l    %d0,&0           
         bgt.w     ssins_restore          
                                    
#--Need to calculate r
         fsub.x    %fp0,%fp3        # A-R
         fadd.x    %fp3,%fp1        # FP1 is r := (A-R)+a
         bra.w     ssins_loop             
                                    
ssins_restore:                            
         fmove.l   %fp2,ssins_n(%a6)      
         move.l    (%a7)+,%d2       
         fmovem.x  (%a7)+,%fp2-%fp5 
                                    
                                    
         move.l    ssins_adjn(%a6),%d0    
         cmpi.l    %d0,&4           
                                    
         blt.w     ssins_sincont          
         bra.b     ssins_sccont           
                                    
ssincosd:                            
#--SIN AND COS OF X FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp1      
         bsr.l     sto_cos          # store cosine result
         bra.l     t_extdnrm        
                                    
ssincos:                            
#--SET ADJN TO 4
         move.l    &4,ssins_adjn(%a6)     
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,ssins_x(%a6)      
         andi.l    &0x7fffffff,%d0  # COMPACTIFY X
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     ssins_scok1            
         bra.w     ssins_scsm             
                                    
ssins_scok1:                              
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     ssins_scmain           
         bra.w     ssins_reducex          
                                    
                                    
ssins_scmain:                             
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
ifdef(`PIC',`	fmul.d	&0x3fe45f306dc9c883,%fp1
',`         fmul.d    ssins_twobypi,%fp1     # X*2/PI
')
                                    
#--HIDE THE NEXT THREE INSTRUCTIONS
ifdef(`PIC',`	lea	pitbl+0x200(%pc),%a1
',`         lea       pitbl+0x200,%a1  # ,32
')
                                    
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,ssins_n(%a6)      # CONVERT TO INTEGER
                                    
         move.l    ssins_n(%a6),%d0       
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # ADDRESS OF N*PIBY2, IN Y1, Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
ssins_sccont:                             
#--continuation point from REDUCEX
                                    
#--HIDE THE NEXT TWO
         move.l    ssins_n(%a6),%d0       
         ror.l     &1,%d0           
                                    
         cmpi.l    %d0,&0           # D0 < 0 IFF N IS ODD
         bge.w     ssins_neven            
                                    
ssins_nodd:                               
#--REGISTERS SAVED SO FAR: D0, A0, FP2.
                                    
         fmove.x   %fp0,ssins_rprime(%a6) 
         fmul.x    %fp0,%fp0        # FP0 IS S = R*R
ifdef(`PIC',`	fmove.d	&0xbd6aaa77ccc994f5,%fp1
',`         fmove.d   ssins_sina7,%fp1       # A7
')
ifdef(`PIC',`	fmove.d	&0x3d2ac4d0d6011ee3,%fp2
',`         fmove.d   ssins_cosb8,%fp2       # B8
')
         fmul.x    %fp0,%fp1        # SA7
         move.l    %d2,-(%a7)       
         move.l    %d0,%d2          
         fmul.x    %fp0,%fp2        # SB8
         ror.l     &1,%d2           
         andi.l    &0x80000000,%d2  
                                    
ifdef(`PIC',`	fadd.d	&0x3de612097aae8da1,%fp1
',`         fadd.d    ssins_sina6,%fp1       # A6+SA7
')
         eor.l     %d0,%d2          
         andi.l    &0x80000000,%d2  
ifdef(`PIC',`	fadd.d	&0xbda9396f9f45ac19,%fp2
',`         fadd.d    ssins_cosb7,%fp2       # B7+SB8
')
                                    
         fmul.x    %fp0,%fp1        # S(A6+SA7)
         eor.l     %d2,ssins_rprime(%a6)  
         move.l    (%a7)+,%d2       
         fmul.x    %fp0,%fp2        # S(B7+SB8)
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
                                    
ifdef(`PIC',`	fadd.d	&0xbe5ae6452a118ae4,%fp1
',`         fadd.d    ssins_sina5,%fp1       # A5+S(A6+SA7)
')
         move.l    &0x3f800000,ssins_posneg1(%a6) 
         eor.l     %d0,ssins_posneg1(%a6) 
ifdef(`PIC',`	fadd.d	&0x3e21eed90612c972,%fp2
',`         fadd.d    ssins_cosb6,%fp2       # B6+S(B7+SB8)
')
                                    
         fmul.x    %fp0,%fp1        # S(A5+S(A6+SA7))
         fmul.x    %fp0,%fp2        # S(B6+S(B7+SB8))
         fmove.x   %fp0,ssins_sprime(%a6) 
                                    
ifdef(`PIC',`	fadd.d	&0x3ec71de3a5341531,%fp1
',`         fadd.d    ssins_sina4,%fp1       # A4+S(A5+S(A6+SA7))
')
         eor.l     %d0,ssins_sprime(%a6)  
ifdef(`PIC',`	fadd.d	&0xbe927e4fb79d9fcf,%fp2
',`         fadd.d    ssins_cosb5,%fp2       # B5+S(B6+S(B7+SB8))
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.d	&0xbf2a01a01a018b59,%fp1
',`         fadd.d    ssins_sina3,%fp1       # )
')
ifdef(`PIC',`	fadd.d	&0x3efa01a01a01d423,%fp2
',`         fadd.d    ssins_cosb4,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.x	&0x3ff8000088888888888859af,%fp1
',`         fadd.x    ssins_sina2,%fp1       # )
')
ifdef(`PIC',`	fadd.x	&0xbff50000b60b60b60b61d438,%fp2
',`         fadd.x    ssins_cosb3,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.x	&0xbffc0000aaaaaaaaaaaaaa99,%fp1
',`         fadd.x    ssins_sina1,%fp1       # )
')
ifdef(`PIC',`	fadd.x	&0x3ffa0000aaaaaaaaaaaaab5e,%fp2
',`         fadd.x    ssins_cosb2,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp2,%fp0        # )
                                    
                                    
                                    
         fmul.x    ssins_rprime(%a6),%fp1 # )
ifdef(`PIC',`	fadd.s	&0xbf000000,%fp0
',`         fadd.s    ssins_cosb1,%fp0       # )
')
         fmul.x    ssins_sprime(%a6),%fp0 # ))
                                    
         move.l    %d1,-(%sp)       # ssins_restore users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fadd.x    ssins_rprime(%a6),%fp1 # COS(X)
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # ssins_restore users exceptions
         fadd.s    ssins_posneg1(%a6),%fp0 # SIN(X)
                                    
         bra.l     t_frcinx         
                                    
                                    
ssins_neven:                              
#--REGISTERS SAVED SO FAR: FP2.
                                    
         fmove.x   %fp0,ssins_rprime(%a6) 
         fmul.x    %fp0,%fp0        # FP0 IS S = R*R
ifdef(`PIC',`	fmove.d	&0x3d2ac4d0d6011ee3,%fp1
',`         fmove.d   ssins_cosb8,%fp1       # B8
')
ifdef(`PIC',`	fmove.d	&0xbd6aaa77ccc994f5,%fp2
',`         fmove.d   ssins_sina7,%fp2       # A7
')
         fmul.x    %fp0,%fp1        # SB8
         fmove.x   %fp0,ssins_sprime(%a6) 
         fmul.x    %fp0,%fp2        # SA7
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
ifdef(`PIC',`	fadd.d	&0xbda9396f9f45ac19,%fp1
',`         fadd.d    ssins_cosb7,%fp1       # B7+SB8
')
ifdef(`PIC',`	fadd.d	&0x3de612097aae8da1,%fp2
',`         fadd.d    ssins_sina6,%fp2       # A6+SA7
')
         eor.l     %d0,ssins_rprime(%a6)  
         eor.l     %d0,ssins_sprime(%a6)  
         fmul.x    %fp0,%fp1        # S(B7+SB8)
         ori.l     &0x3f800000,%d0  
         move.l    %d0,ssins_posneg1(%a6) 
         fmul.x    %fp0,%fp2        # S(A6+SA7)
                                    
ifdef(`PIC',`	fadd.d	&0x3e21eed90612c972,%fp1
',`         fadd.d    ssins_cosb6,%fp1       # B6+S(B7+SB8)
')
ifdef(`PIC',`	fadd.d	&0xbe5ae6452a118ae4,%fp2
',`         fadd.d    ssins_sina5,%fp2       # A5+S(A6+SA7)
')
                                    
         fmul.x    %fp0,%fp1        # S(B6+S(B7+SB8))
         fmul.x    %fp0,%fp2        # S(A5+S(A6+SA7))
                                    
ifdef(`PIC',`	fadd.d	&0xbe927e4fb79d9fcf,%fp1
',`         fadd.d    ssins_cosb5,%fp1       # B5+S(B6+S(B7+SB8))
')
ifdef(`PIC',`	fadd.d	&0x3ec71de3a5341531,%fp2
',`         fadd.d    ssins_sina4,%fp2       # A4+S(A5+S(A6+SA7))
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.d	&0x3efa01a01a01d423,%fp1
',`         fadd.d    ssins_cosb4,%fp1       # )
')
ifdef(`PIC',`	fadd.d	&0xbf2a01a01a018b59,%fp2
',`         fadd.d    ssins_sina3,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.x	&0xbff50000b60b60b60b61d438,%fp1
',`         fadd.x    ssins_cosb3,%fp1       # )
')
ifdef(`PIC',`	fadd.x	&0x3ff8000088888888888859af,%fp2
',`         fadd.x    ssins_sina2,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
ifdef(`PIC',`	fadd.x	&0x3ffa0000aaaaaaaaaaaaab5e,%fp1
',`         fadd.x    ssins_cosb2,%fp1       # )
')
ifdef(`PIC',`	fadd.x	&0xbffc0000aaaaaaaaaaaaaa99,%fp2
',`         fadd.x    ssins_sina1,%fp2       # )
')
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp2,%fp0        # )
                                    
                                    
                                    
ifdef(`PIC',`	fadd.s	&0xbf000000,%fp1
',`         fadd.s    ssins_cosb1,%fp1       # )
')
         fmul.x    ssins_rprime(%a6),%fp0 # )
         fmul.x    ssins_sprime(%a6),%fp1 # ))
                                    
         move.l    %d1,-(%sp)       # save users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fadd.s    ssins_posneg1(%a6),%fp1 # COS(X)
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # ssins_restore users exceptions
         fadd.x    ssins_rprime(%a6),%fp0 # SIN(X)
                                    
         bra.l     t_frcinx         
                                    
ssins_scbors:                             
         cmpi.l    %d0,&0x3fff8000  
         bgt.w     ssins_reducex          
                                    
                                    
ssins_scsm:                               
         move.w    &0x0000,ssins_xdcare(%a6) 
         fmove.s   &0f1.0,%fp1      
                                    
         move.l    %d1,-(%sp)       # save users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fsub.s    &0f1.1754943e-38,%fp1 
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # ssins_restore users exceptions
         fmove.x   ssins_x(%a6),%fp0      
         bra.l     t_frcinx         
                                    
                                    
	version 3
ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /misc/source_product/9.10/commands.rcs/lib/libm/fp040_trig.s,v $
# $Revision: 70.2 $      $Author: ssa $
# $State: Exp $         $Locker:  $
# $Date: 91/08/30 00:10:56 $
                                    
#
#	stan.sa 3.3 7/29/91
#
#	The entry point stan computes the tangent of
#	an input argument;
#	stand does the same except for denormalized input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value tan(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulp in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program sTAN takes approximately 170 cycles for
#		input argument X such that |X| < 15Pi, which is the the usual
#		situation.
#
#	Algorithm:
#
#	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.
#
#	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 2, so in particular, k = 0 or 1.
#
#	3. If k is odd, go to 5.
#
#	4. (k is even) Tan(X) = tan(r) and tan(r) is approximated by a
#		rational function U/V where
#		U = r + r*s*(P1 + s*(P2 + s*P3)), and
#		V = 1 + s*(Q1 + s*(Q2 + s*(Q3 + s*Q4))),  s = r*r.
#		Exit.
#
#	4. (k is odd) Tan(X) = -cot(r). Since tan(r) is approximated by a
#		rational function U/V where
#		U = r + r*s*(P1 + s*(P2 + s*P3)), and
#		V = 1 + s*(Q1 + s*(Q2 + s*(Q3 + s*Q4))), s = r*r,
#		-Cot(r) = -V/U. Exit.
#
#	6. If |X| > 1, go to 8.
#
#	7. (|X|<2**(-40)) Tan(X) = X. Exit.
#
#	8. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 2.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
stans_bounds1: long      0x3fd78000,0x4004bc7e 
stans_twobypi: long      0x3fe45f30,0x6dc9c883 
                                    
stans_tanq4:   long      0x3ea0b759,0xf50f8688 
stans_tanp3:   long      0xbef2baa5,0xa8924f04 
                                    
stans_tanq3:   long      0xbf346f59,0xb39ba65f,0x00000000,0x00000000 
                                    
stans_tanp2:   long      0x3ff60000,0xe073d3fc,0x199c4a00,0x00000000 
                                    
stans_tanq2:   long      0x3ff90000,0xd23cd684,0x15d95fa1,0x00000000 
                                    
stans_tanp1:   long      0xbffc0000,0x8895a6c5,0xfb423bca,0x00000000 
                                    
stans_tanq1:   long      0xbffd0000,0xeef57e0d,0xa84bc8ce,0x00000000 
                                    
stans_invtwopi: long      0x3ffc0000,0xa2f9836e,0x4e44152a,0x00000000 
                                    
stans_twopi1:  long      0x40010000,0xc90fdaa2,0x00000000,0x00000000 
stans_twopi2:  long      0x3fdf0000,0x85a308d4,0x00000000,0x00000000 
                                    
#--N*PI/2, -32 <= N <= 32, IN A LEADING TERM IN EXT. AND TRAILING
#--TERM IN SGL. NOTE THAT PI IS 64-BIT LONG, THUS N*PI/2 IS AT
#--MOST 69 BITS LONG.
pitbl:                              
         long      0xc0040000,0xc90fdaa2,0x2168c235,0x21800000 
         long      0xc0040000,0xc2c75bcd,0x105d7c23,0xa0d00000 
         long      0xc0040000,0xbc7edcf7,0xff523611,0xa1e80000 
         long      0xc0040000,0xb6365e22,0xee46f000,0x21480000 
         long      0xc0040000,0xafeddf4d,0xdd3ba9ee,0xa1200000 
         long      0xc0040000,0xa9a56078,0xcc3063dd,0x21fc0000 
         long      0xc0040000,0xa35ce1a3,0xbb251dcb,0x21100000 
         long      0xc0040000,0x9d1462ce,0xaa19d7b9,0xa1580000 
         long      0xc0040000,0x96cbe3f9,0x990e91a8,0x21e00000 
         long      0xc0040000,0x90836524,0x88034b96,0x20b00000 
         long      0xc0040000,0x8a3ae64f,0x76f80584,0xa1880000 
         long      0xc0040000,0x83f2677a,0x65ecbf73,0x21c40000 
         long      0xc0030000,0xfb53d14a,0xa9c2f2c2,0x20000000 
         long      0xc0030000,0xeec2d3a0,0x87ac669f,0x21380000 
         long      0xc0030000,0xe231d5f6,0x6595da7b,0xa1300000 
         long      0xc0030000,0xd5a0d84c,0x437f4e58,0x9fc00000 
         long      0xc0030000,0xc90fdaa2,0x2168c235,0x21000000 
         long      0xc0030000,0xbc7edcf7,0xff523611,0xa1680000 
         long      0xc0030000,0xafeddf4d,0xdd3ba9ee,0xa0a00000 
         long      0xc0030000,0xa35ce1a3,0xbb251dcb,0x20900000 
         long      0xc0030000,0x96cbe3f9,0x990e91a8,0x21600000 
         long      0xc0030000,0x8a3ae64f,0x76f80584,0xa1080000 
         long      0xc0020000,0xfb53d14a,0xa9c2f2c2,0x1f800000 
         long      0xc0020000,0xe231d5f6,0x6595da7b,0xa0b00000 
         long      0xc0020000,0xc90fdaa2,0x2168c235,0x20800000 
         long      0xc0020000,0xafeddf4d,0xdd3ba9ee,0xa0200000 
         long      0xc0020000,0x96cbe3f9,0x990e91a8,0x20e00000 
         long      0xc0010000,0xfb53d14a,0xa9c2f2c2,0x1f000000 
         long      0xc0010000,0xc90fdaa2,0x2168c235,0x20000000 
         long      0xc0010000,0x96cbe3f9,0x990e91a8,0x20600000 
         long      0xc0000000,0xc90fdaa2,0x2168c235,0x1f800000 
         long      0xbfff0000,0xc90fdaa2,0x2168c235,0x1f000000 
         long      0x00000000,0x00000000,0x00000000,0x00000000 
         long      0x3fff0000,0xc90fdaa2,0x2168c235,0x9f000000 
         long      0x40000000,0xc90fdaa2,0x2168c235,0x9f800000 
         long      0x40010000,0x96cbe3f9,0x990e91a8,0xa0600000 
         long      0x40010000,0xc90fdaa2,0x2168c235,0xa0000000 
         long      0x40010000,0xfb53d14a,0xa9c2f2c2,0x9f000000 
         long      0x40020000,0x96cbe3f9,0x990e91a8,0xa0e00000 
         long      0x40020000,0xafeddf4d,0xdd3ba9ee,0x20200000 
         long      0x40020000,0xc90fdaa2,0x2168c235,0xa0800000 
         long      0x40020000,0xe231d5f6,0x6595da7b,0x20b00000 
         long      0x40020000,0xfb53d14a,0xa9c2f2c2,0x9f800000 
         long      0x40030000,0x8a3ae64f,0x76f80584,0x21080000 
         long      0x40030000,0x96cbe3f9,0x990e91a8,0xa1600000 
         long      0x40030000,0xa35ce1a3,0xbb251dcb,0xa0900000 
         long      0x40030000,0xafeddf4d,0xdd3ba9ee,0x20a00000 
         long      0x40030000,0xbc7edcf7,0xff523611,0x21680000 
         long      0x40030000,0xc90fdaa2,0x2168c235,0xa1000000 
         long      0x40030000,0xd5a0d84c,0x437f4e58,0x1fc00000 
         long      0x40030000,0xe231d5f6,0x6595da7b,0x21300000 
         long      0x40030000,0xeec2d3a0,0x87ac669f,0xa1380000 
         long      0x40030000,0xfb53d14a,0xa9c2f2c2,0xa0000000 
         long      0x40040000,0x83f2677a,0x65ecbf73,0xa1c40000 
         long      0x40040000,0x8a3ae64f,0x76f80584,0x21880000 
         long      0x40040000,0x90836524,0x88034b96,0xa0b00000 
         long      0x40040000,0x96cbe3f9,0x990e91a8,0xa1e00000 
         long      0x40040000,0x9d1462ce,0xaa19d7b9,0x21580000 
         long      0x40040000,0xa35ce1a3,0xbb251dcb,0xa1100000 
         long      0x40040000,0xa9a56078,0xcc3063dd,0xa1fc0000 
         long      0x40040000,0xafeddf4d,0xdd3ba9ee,0x21200000 
         long      0x40040000,0xb6365e22,0xee46f000,0xa1480000 
         long      0x40040000,0xbc7edcf7,0xff523611,0x21e80000 
         long      0x40040000,0xc2c75bcd,0x105d7c23,0x20d00000 
         long      0x40040000,0xc90fdaa2,0x2168c235,0xa1800000 
                                    
         set       stans_inarg,fp_scr4    
                                    
         set       stans_twoto63,l_scr1   
         set       stans_endflag,l_scr2   
         set       stans_n,l_scr3         
                                    
                                    
                                    
                                    
stand:                              
#--TAN(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
stan:                               
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     stans_tanok1           
         bra.w     stans_tansm            
stans_tanok1:                             
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     stans_tanmain          
         bra.w     stans_reducex          
                                    
                                    
stans_tanmain:                            
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
ifdef(`PIC',`	fmul.d	&0x3fe45f306dc9c883,%fp1
',`         fmul.d    stans_twobypi,%fp1     # X*2/PI
')
                                    
#--HIDE THE NEXT TWO INSTRUCTIONS
ifdef(`PIC',`	lea.l	pitbl+0x200(%pc),%a1
',`         lea.l     pitbl+0x200,%a1  # ,32
')
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,%d0         # CONVERT TO INTEGER
                                    
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # ADDRESS N*PIBY2 IN Y1, Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
#--HIDE THE NEXT ONE
                                    
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
         ror.l     &5,%d0           
         andi.l    &0x80000000,%d0  # D0 WAS ODD IFF D0 < 0
                                    
stans_tancont:                            
                                    
         cmpi.l    %d0,&0           
         blt.w     stans_nodd             
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # S = R*R
                                    
ifdef(`PIC',`	fmove.d	&0x3ea0b759f50f8688,%fp3
',`         fmove.d   stans_tanq4,%fp3       
')
ifdef(`PIC',`	fmove.d	&0xbef2baa5a8924f04,%fp2
',`         fmove.d   stans_tanp3,%fp2       
')
                                    
         fmul.x    %fp1,%fp3        # SQ4
         fmul.x    %fp1,%fp2        # SP3
                                    
ifdef(`PIC',`	fadd.d	&0xbf346f59b39ba65f,%fp3
',`         fadd.d    stans_tanq3,%fp3       # Q3+SQ4
')
ifdef(`PIC',`	fadd.x	&0x3ff60000e073d3fc199c4a00,%fp2
',`         fadd.x    stans_tanp2,%fp2       # P2+SP3
')
                                    
         fmul.x    %fp1,%fp3        # S(Q3+SQ4)
         fmul.x    %fp1,%fp2        # S(P2+SP3)
                                    
ifdef(`PIC',`	fadd.x	&0x3ff90000d23cd68415d95fa1,%fp3
',`         fadd.x    stans_tanq2,%fp3       # Q2+S(Q3+SQ4)
')
ifdef(`PIC',`	fadd.x	&0xbffc00008895a6c5fb423bca,%fp2
',`         fadd.x    stans_tanp1,%fp2       # P1+S(P2+SP3)
')
                                    
         fmul.x    %fp1,%fp3        # S(Q2+S(Q3+SQ4))
         fmul.x    %fp1,%fp2        # S(P1+S(P2+SP3))
                                    
ifdef(`PIC',`	fadd.x	&0xbffd0000eef57e0da84bc8ce,%fp3
',`         fadd.x    stans_tanq1,%fp3       # Q1+S(Q2+S(Q3+SQ4))
')
         fmul.x    %fp0,%fp2        # RS(P1+S(P2+SP3))
                                    
         fmul.x    %fp3,%fp1        # S(Q1+S(Q2+S(Q3+SQ4)))
                                    
                                    
         fadd.x    %fp2,%fp0        # R+RS(P1+S(P2+SP3))
                                    
                                    
         fadd.s    &0f1.0,%fp1      # )
                                    
         fmove.l   %d1,%fpcr        # stans_restore users exceptions
         fdiv.x    %fp1,%fp0        # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
stans_nodd:                               
         fmove.x   %fp0,%fp1        
         fmul.x    %fp0,%fp0        # S = R*R
                                    
ifdef(`PIC',`	fmove.d	&0x3ea0b759f50f8688,%fp3
',`         fmove.d   stans_tanq4,%fp3       
')
ifdef(`PIC',`	fmove.d	&0xbef2baa5a8924f04,%fp2
',`         fmove.d   stans_tanp3,%fp2       
')
                                    
         fmul.x    %fp0,%fp3        # SQ4
         fmul.x    %fp0,%fp2        # SP3
                                    
ifdef(`PIC',`	fadd.d	&0xbf346f59b39ba65f,%fp3
',`         fadd.d    stans_tanq3,%fp3       # Q3+SQ4
')
ifdef(`PIC',`	fadd.x	&0x3ff60000e073d3fc199c4a00,%fp2
',`         fadd.x    stans_tanp2,%fp2       # P2+SP3
')
                                    
         fmul.x    %fp0,%fp3        # S(Q3+SQ4)
         fmul.x    %fp0,%fp2        # S(P2+SP3)
                                    
ifdef(`PIC',`	fadd.x	&0x3ff90000d23cd68415d95fa1,%fp3
',`         fadd.x    stans_tanq2,%fp3       # Q2+S(Q3+SQ4)
')
ifdef(`PIC',`	fadd.x	&0xbffc00008895a6c5fb423bca,%fp2
',`         fadd.x    stans_tanp1,%fp2       # P1+S(P2+SP3)
')
                                    
         fmul.x    %fp0,%fp3        # S(Q2+S(Q3+SQ4))
         fmul.x    %fp0,%fp2        # S(P1+S(P2+SP3))
                                    
ifdef(`PIC',`	fadd.x	&0xbffd0000eef57e0da84bc8ce,%fp3
',`         fadd.x    stans_tanq1,%fp3       # Q1+S(Q2+S(Q3+SQ4))
')
         fmul.x    %fp1,%fp2        # RS(P1+S(P2+SP3))
                                    
         fmul.x    %fp3,%fp0        # S(Q1+S(Q2+S(Q3+SQ4)))
                                    
                                    
         fadd.x    %fp2,%fp1        # R+RS(P1+S(P2+SP3))
         fadd.s    &0f1.0,%fp0      # )
                                    
                                    
         fmove.x   %fp1,-(%sp)      
         eori.l    &0x80000000,(%sp) 
                                    
         fmove.l   %d1,%fpcr        # stans_restore users exceptions
         fdiv.x    (%sp)+,%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
stans_tanbors:                            
#--IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION.
#--IF |X| < 2**(-40), RETURN X OR 1.
         cmpi.l    %d0,&0x3fff8000  
         bgt.b     stans_reducex          
                                    
stans_tansm:                              
                                    
         fmove.x   %fp0,-(%sp)      
         fmove.l   %d1,%fpcr        # stans_restore users exceptions
         fmove.x   (%sp)+,%fp0      # last inst - posibble exception set
                                    
         bra.l     t_frcinx         
                                    
                                    
stans_reducex:                            
#--WHEN REDUCEX IS USED, THE CODE WILL INEVITABLY BE SLOW.
#--THIS REDUCTION METHOD, HOWEVER, IS MUCH FASTER THAN USING
#--THE REMAINDER INSTRUCTION WHICH IS NOW IN SOFTWARE.
                                    
         fmovem.x  %fp2-%fp5,-(%a7) # save FP2 through FP5
         move.l    %d2,-(%a7)       
         fmove.s   &0f0.0,%fp1      
                                    
#--If compact form of abs(arg) in d0=$7ffeffff, argument is so large that
#--there is a danger of unwanted overflow in first LOOP iteration.  In this
#--case, reduce argument by one remainder step to make subsequent reduction
#--safe.
         cmpi.l    %d0,&0x7ffeffff  # is argument dangerously large?
         bne.b     stans_loop             
         move.l    &0x7ffe0000,fp_scr2(%a6) # yes
#					;create 2**16383*PI/2
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   
         ftest.x   %fp0             # test sign of argument
         move.l    &0x7fdc0000,fp_scr3(%a6) # create low half of 2**16383*
#					;PI/2 at FP_SCR3
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   
         fblt.w    stans_red_neg          
         or.w      &0x8000,fp_scr2(%a6) # positive arg
         or.w      &0x8000,fp_scr3(%a6) 
stans_red_neg:                            
         fadd.x    fp_scr2(%a6),%fp0 # high part of reduction is exact
         fmove.x   %fp0,%fp1        # save high result in fp1
         fadd.x    fp_scr3(%a6),%fp0 # low part of reduction
         fsub.x    %fp0,%fp1        # determine low component of result
         fadd.x    fp_scr3(%a6),%fp1 # fp0/fp1 are reduced argument.
                                    
#--ON ENTRY, FP0 IS X, ON RETURN, FP0 IS X REM PI/2, |X| <= PI/4.
#--integer quotient will be stored in N
#--Intermeditate remainder is 66-bit long; (R,r) in (FP0,FP1)
                                    
stans_loop:                               
         fmove.x   %fp0,stans_inarg(%a6)  # +-2**K * F, 1 <= F < 2
         move.w    stans_inarg(%a6),%d0   
         move.l    %d0,%a1          # save a copy of D0
         andi.l    &0x00007fff,%d0  
         subi.l    &0x00003fff,%d0  # D0 IS K
         cmpi.l    %d0,&28          
         ble.b     stans_lastloop         
stans_contloop:                            
         subi.l    &27,%d0          # D0 IS L := K-27
         move.l    &0,stans_endflag(%a6)  
         bra.b     stans_work             
stans_lastloop:                            
         clr.l     %d0              # D0 IS L := 0
         move.l    &1,stans_endflag(%a6)  
                                    
stans_work:                               
#--FIND THE REMAINDER OF (R,r) W.R.T.	2**L * (PI/2). L IS SO CHOSEN
#--THAT	INT( X * (2/PI) / 2**(L) ) < 2**29.
                                    
#--CREATE 2**(-L) * (2/PI), SIGN(INARG)*2**(63),
#--2**L * (PIby2_1), 2**L * (PIby2_2)
                                    
         move.l    &0x00003ffe,%d2  # BIASED EXPO OF 2/PI
         sub.l     %d0,%d2          # BIASED EXPO OF 2**(-L)*(2/PI)
                                    
         move.l    &0xa2f9836e,fp_scr1+4(%a6) 
         move.l    &0x4e44152a,fp_scr1+8(%a6) 
         move.w    %d2,fp_scr1(%a6) # FP_SCR1 is 2**(-L)*(2/PI)
                                    
         fmove.x   %fp0,%fp2        
         fmul.x    fp_scr1(%a6),%fp2 
#--WE MUST NOW FIND INT(FP2). SINCE WE NEED THIS VALUE IN
#--FLOATING POINT FORMAT, THE TWO FMOVE'S	FMOVE.L FP <--> N
#--WILL BE TOO INEFFICIENT. THE WAY AROUND IT IS THAT
#--(SIGN(INARG)*2**63	+	FP2) - SIGN(INARG)*2**63 WILL GIVE
#--US THE DESIRED VALUE IN FLOATING POINT.
                                    
#--HIDE SIX CYCLES OF INSTRUCTION
         move.l    %a1,%d2          
         swap      %d2              
         andi.l    &0x80000000,%d2  
         ori.l     &0x5f000000,%d2  # D2 IS SIGN(INARG)*2**63 IN SGL
         move.l    %d2,stans_twoto63(%a6) 
                                    
         move.l    %d0,%d2          
         addi.l    &0x00003fff,%d2  # BIASED EXPO OF 2**L * (PI/2)
                                    
#--FP2 IS READY
         fadd.s    stans_twoto63(%a6),%fp2 # THE FRACTIONAL PART OF FP1 IS ROUNDED
                                    
#--HIDE 4 CYCLES OF INSTRUCTION; creating 2**(L)*Piby2_1  and  2**(L)*Piby2_2
         move.w    %d2,fp_scr2(%a6) 
         clr.w     fp_scr2+2(%a6)   
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   # FP_SCR2 is  2**(L) * Piby2_1	
                                    
#--FP2 IS READY
         fsub.s    stans_twoto63(%a6),%fp2 # FP2 is N
                                    
         addi.l    &0x00003fdd,%d0  
         move.w    %d0,fp_scr3(%a6) 
         clr.w     fp_scr3+2(%a6)   
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   # FP_SCR3 is 2**(L) * Piby2_2
                                    
         move.l    stans_endflag(%a6),%d0 
                                    
#--We are now ready to perform (R+r) - N*P1 - N*P2, P1 = 2**(L) * Piby2_1 and
#--P2 = 2**(L) * Piby2_2
         fmove.x   %fp2,%fp4        
         fmul.x    fp_scr2(%a6),%fp4 # W = N*P1
         fmove.x   %fp2,%fp5        
         fmul.x    fp_scr3(%a6),%fp5 # w = N*P2
         fmove.x   %fp4,%fp3        
#--we want P+p = W+w  but  |p| <= half ulp of P
#--Then, we need to compute  A := R-P   and  a := r-p
         fadd.x    %fp5,%fp3        # FP3 is P
         fsub.x    %fp3,%fp4        # W-P
                                    
         fsub.x    %fp3,%fp0        # FP0 is A := R - P
         fadd.x    %fp5,%fp4        # FP4 is p = (W-P)+w
                                    
         fmove.x   %fp0,%fp3        # FP3 A
         fsub.x    %fp4,%fp1        # FP1 is a := r - p
                                    
#--Now we need to normalize (A,a) to  "new (R,r)" where R+r = A+a but
#--|r| <= half ulp of R.
         fadd.x    %fp1,%fp0        # FP0 is R := A+a
#--No need to calculate r if this is the last stans_loop
         cmpi.l    %d0,&0           
         bgt.w     stans_restore          
                                    
#--Need to calculate r
         fsub.x    %fp0,%fp3        # A-R
         fadd.x    %fp3,%fp1        # FP1 is r := (A-R)+a
         bra.w     stans_loop             
                                    
stans_restore:                            
         fmove.l   %fp2,stans_n(%a6)      
         move.l    (%a7)+,%d2       
         fmovem.x  (%a7)+,%fp2-%fp5 
                                    
                                    
         move.l    stans_n(%a6),%d0       
         ror.l     &1,%d0           
                                    
                                    
         bra.w     stans_tancont          
                                    
                                    
	version 3
