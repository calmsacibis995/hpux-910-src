ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/scale.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:50:53 $
                                    
#
#	scale.sa 3.3 7/30/91
#
#	The entry point sSCALE computes the destination operand
#	scaled by the source operand.  If the absoulute value of
#	the source operand is (>= 2^14) an overflow or underflow
#	is returned.
#
#	The entry point sscale is called from do_func to emulate
#	the fscale unimplemented instruction.
#
#	Input: Double-extended destination operand in FPTEMP, 
#		double-extended source operand in ETEMP.
#
#	Output: The function returns scale(X,Y) to fp0.
#
#	Modifies: fp0.
#
#	Algorithm:
#		
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
src_bnds: short     0x3fff,0x400c    
                                    
#
# This entry point is used by the unimplemented instruction exception
# handler.
#
#
#
#	FSCALE
#
         global    sscale           
sscale:                             
         fmove.l   &0,%fpcr         # clr user enabled exc
         clr.l     %d1              
         move.w    fptemp(%a6),%d1  # get dest exponent
         smi       l_scr1(%a6)      # use L_SCR1 to hold sign
         andi.l    &0x7fff,%d1      # strip sign
         move.w    etemp(%a6),%d0   # check src bounds
         andi.w    &0x7fff,%d0      # clr sign bit
         cmp2.w    %d0,src_bnds     
         bcc.b     src_in           
         cmpi.w    %d0,&0x400c      # test for too large
         bge.w     src_out          
#
# The source input is below 1, so we check for denormalized numbers
# and set unfl.
#
src_small:                            
         move.b    dtag(%a6),%d0    
         andi.b    &0xe0,%d0        
         tst.b     %d0              
         beq.b     no_denorm        
         st        store_flg(%a6)   # dest already contains result
         or.l      &unfl_mask,user_fpsr(%a6) # set UNFL
den_done:                            
         lea.l     fptemp(%a6),%a0  
         bra.l     t_resdnrm        
no_denorm:                            
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 # simply return dest
         rts                        
                                    
                                    
#
# Source is within 2^14 range.  To perform the int operation,
# move it to d0.
#
src_in:                             
         fmove.x   etemp(%a6),%fp0  # move in src for int
         fmove.l   &rz_mode,%fpcr   # force rz for src conversion
         fmove.l   %fp0,%d0         # int src to d0
         fmove.l   &0,%fpsr         # clr status from above
         tst.w     etemp(%a6)       # check src sign
         blt.w     src_neg          
#
# Source is positive.  Add the src to the dest exponent.
# The result can be denormalized, if src = 0, or overflow,
# if the result of the add sets a bit in the upper word.
#
src_pos:                            
         tst.w     %d1              # check for denorm
         beq.w     dst_dnrm         
         add.l     %d0,%d1          # add src to dest exp
         beq.b     denorm           # if zero, result is denorm
         cmpi.l    %d1,&0x7fff      # test for overflow
         bge.b     ovfl             
         tst.b     l_scr1(%a6)      
         beq.b     spos_pos         
         or.w      &0x8000,%d1      
spos_pos:                            
         move.w    %d1,fptemp(%a6)  # result in FPTEMP
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 # write result to fp0
         rts                        
ovfl:                               
         tst.b     l_scr1(%a6)      
         beq.b     sovl_pos         
         or.w      &0x8000,%d1      
sovl_pos:                            
         move.w    fptemp(%a6),etemp(%a6) # result in ETEMP
         move.l    fptemp_hi(%a6),etemp_hi(%a6) 
         move.l    fptemp_lo(%a6),etemp_lo(%a6) 
         bra.l     t_ovfl2          
                                    
denorm:                             
         tst.b     l_scr1(%a6)      
         beq.b     den_pos          
         or.w      &0x8000,%d1      
den_pos:                            
         tst.l     fptemp_hi(%a6)   # check j bit
         blt.b     nden_exit        # if set, not denorm
         move.w    %d1,etemp(%a6)   # input expected in ETEMP
         move.l    fptemp_hi(%a6),etemp_hi(%a6) 
         move.l    fptemp_lo(%a6),etemp_lo(%a6) 
         or.l      &unfl_bit,user_fpsr(%a6) # set unfl
         lea.l     etemp(%a6),%a0   
         bra.l     t_resdnrm        
nden_exit:                            
         move.w    %d1,fptemp(%a6)  # result in FPTEMP
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 # write result to fp0
         rts                        
                                    
#
# Source is negative.  Add the src to the dest exponent.
# (The result exponent will be reduced).  The result can be
# denormalized.
#
src_neg:                            
         add.l     %d0,%d1          # add src to dest
         beq.b     denorm           # if zero, result is denorm
         blt.b     fix_dnrm         # if negative, result is 
#					;needing denormalization
         tst.b     l_scr1(%a6)      
         beq.b     sneg_pos         
         or.w      &0x8000,%d1      
sneg_pos:                            
         move.w    %d1,fptemp(%a6)  # result in FPTEMP
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 # write result to fp0
         rts                        
                                    
                                    
#
# The result exponent is below denorm value.  Test for catastrophic
# underflow and force zero if true.  If not, try to shift the 
# mantissa right until a zero exponent exists.
#
fix_dnrm:                            
         cmpi.w    %d1,&0xffc0      # lower bound for normalization
         blt.w     fix_unfl         # if lower, catastrophic unfl
         move.w    %d1,%d0          # use d0 for exp
         move.l    %d2,-(%a7)       # free d2 for norm
         move.l    fptemp_hi(%a6),%d1 
         move.l    fptemp_lo(%a6),%d2 
         clr.l     l_scr2(%a6)      
fix_loop:                            
         add.w     &1,%d0           # drive d0 to 0
         lsr.l     &1,%d1           # while shifting the
         roxr.l    &1,%d2           # mantissa to the right
         bcc.b     no_carry         
         st        l_scr2(%a6)      # use L_SCR2 to capture inex
no_carry:                            
         tst.w     %d0              # it is finished when
         blt.b     fix_loop         # d0 is zero or the mantissa
         tst.b     l_scr2(%a6)      
         beq.b     tst_zero         
         or.l      &unfl_inx_mask,user_fpsr(%a6) 
#					;set unfl, aunfl, ainex
#
# Test for zero. If zero, simply use fmove to return +/- zero
# to the fpu.
#
tst_zero:                            
         clr.w     fptemp_ex(%a6)   
         tst.b     l_scr1(%a6)      # test for sign
         beq.b     tst_con          
         or.w      &0x8000,fptemp_ex(%a6) # set sign bit
tst_con:                            
         move.l    %d1,fptemp_hi(%a6) 
         move.l    %d2,fptemp_lo(%a6) 
         move.l    (%a7)+,%d2       
         tst.l     %d1              
         bne.b     not_zero         
         tst.l     fptemp_lo(%a6)   
         bne.b     not_zero         
#
# Result is zero.  Check for rounding mode to set lsb.  If the
# mode is rp, and the zero is positive, return smallest denorm.
# If the mode is rm, and the zero is negative, return smallest
# negative denorm.
#
         btst.b    &5,fpcr_mode(%a6) # test if rm or rp
         beq.b     no_dir           
         btst.b    &4,fpcr_mode(%a6) # check which one
         beq.b     zer_rm           
zer_rp:                             
         tst.b     l_scr1(%a6)      # check sign
         bne.b     no_dir           # if set, neg op, no inc
         move.l    &1,fptemp_lo(%a6) # set lsb
         bra.b     sm_dnrm          
zer_rm:                             
         tst.b     l_scr1(%a6)      # check sign
         beq.b     no_dir           # if clr, neg op, no inc
         move.l    &1,fptemp_lo(%a6) # set lsb
         or.l      &neg_mask,user_fpsr(%a6) # set N
         bra.b     sm_dnrm          
no_dir:                             
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 # use fmove to set cc's
         rts                        
                                    
#
# The rounding mode changed the zero to a smallest denorm. Call 
# t_resdnrm with exceptional operand in ETEMP.
#
sm_dnrm:                            
         move.l    fptemp_ex(%a6),etemp_ex(%a6) 
         move.l    fptemp_hi(%a6),etemp_hi(%a6) 
         move.l    fptemp_lo(%a6),etemp_lo(%a6) 
         lea.l     etemp(%a6),%a0   
         bra.l     t_resdnrm        
                                    
#
# Result is still denormalized.
#
not_zero:                            
         or.l      &unfl_mask,user_fpsr(%a6) # set unfl
         tst.b     l_scr1(%a6)      # check for sign
         beq.b     fix_exit         
         or.l      &neg_mask,user_fpsr(%a6) # set N
fix_exit:                            
         bra.b     sm_dnrm          
                                    
                                    
#
# The result has underflowed to zero. Return zero and set
# unfl, aunfl, and ainex.
#
fix_unfl:                            
         or.l      &unfl_inx_mask,user_fpsr(%a6) 
         btst.b    &5,fpcr_mode(%a6) # test if rm or rp
         beq.b     no_dir2          
         btst.b    &4,fpcr_mode(%a6) # check which one
         beq.b     zer_rm2          
zer_rp2:                            
         tst.b     l_scr1(%a6)      # check sign
         bne.b     no_dir2          # if set, neg op, no inc
         clr.l     fptemp_ex(%a6)   
         clr.l     fptemp_hi(%a6)   
         move.l    &1,fptemp_lo(%a6) # set lsb
         bra.b     sm_dnrm          # return smallest denorm
zer_rm2:                            
         tst.b     l_scr1(%a6)      # check sign
         beq.b     no_dir2          # if clr, neg op, no inc
         move.w    &0x8000,fptemp_ex(%a6) 
         clr.l     fptemp_hi(%a6)   
         move.l    &1,fptemp_lo(%a6) # set lsb
         or.l      &neg_mask,user_fpsr(%a6) # set N
         bra.w     sm_dnrm          # return smallest denorm
                                    
no_dir2:                            
         tst.b     l_scr1(%a6)      
         bge.b     pos_zero         
neg_zero:                            
         clr.l     fp_scr1(%a6)     # clear the exceptional operand
         clr.l     fp_scr1+4(%a6)   # for gen_except.
         clr.l     fp_scr1+8(%a6)   
         fmove.s   &0x80000000,%fp0 
         rts                        
pos_zero:                            
         clr.l     fp_scr1(%a6)     # clear the exceptional operand
         clr.l     fp_scr1+4(%a6)   # for gen_except.
         clr.l     fp_scr1+8(%a6)   
         fmove.s   &0f0.0,%fp0      
         rts                        
                                    
#
# The destination is a denormalized number.  It must be handled
# by first shifting the bits in the mantissa until it is normalized,
# then adding the remainder of the source to the exponent.
#
dst_dnrm:                            
         movem.l   %d2/%d3,-(%a7)   
         move.w    fptemp_ex(%a6),%d1 
         move.l    fptemp_hi(%a6),%d2 
         move.l    fptemp_lo(%a6),%d3 
dst_loop:                            
         tst.l     %d2              # test for normalized result
         blt.b     dst_norm         # exit loop if so
         tst.l     %d0              # otherwise, test shift count
         beq.b     dst_fin          # if zero, shifting is done
         subi.l    &1,%d0           # dec src
         lsl.l     &1,%d3           
         roxl.l    &1,%d2           
         bra.b     dst_loop         
#
# Destination became normalized.  Simply add the remaining 
# portion of the src to the exponent.
#
dst_norm:                            
         add.w     %d0,%d1          #  add src
         tst.b     l_scr1(%a6)      
         beq.b     dnrm_pos         
         or.l      &0x8000,%d1      
dnrm_pos:                            
         movem.w   %d1,fptemp_ex(%a6) 
         movem.l   %d2,fptemp_hi(%a6) 
         movem.l   %d3,fptemp_lo(%a6) 
         fmove.l   user_fpcr(%a6),%fpcr 
         fmove.x   fptemp(%a6),%fp0 
         movem.l   (%a7)+,%d2/%d3   
         rts                        
                                    
#
# Destination remained denormalized.  Call t_excdnrm with
# exceptional operand in ETEMP.
#
dst_fin:                            
         tst.b     l_scr1(%a6)      # check for sign
         beq.b     dst_exit         
         or.l      &neg_mask,user_fpsr(%a6) # set N
         or.l      &0x8000,%d1      
dst_exit:                            
         movem.w   %d1,etemp_ex(%a6) 
         movem.l   %d2,etemp_hi(%a6) 
         movem.l   %d3,etemp_lo(%a6) 
         or.l      &unfl_mask,user_fpsr(%a6) # set unfl
         movem.l   (%a7)+,%d2/%d3   
         lea.l     etemp(%a6),%a0   
         bra.l     t_resdnrm        
                                    
#
# Source is outside of 2^14 range.  Test the sign and branch
# to the appropriate exception handler.
#
src_out:                            
         tst.b     l_scr1(%a6)      
         beq.b     scro_pos         
         or.l      &0x8000,%d1      
scro_pos:                            
         move.l    fptemp_hi(%a6),etemp_hi(%a6) 
         move.l    fptemp_lo(%a6),etemp_lo(%a6) 
         tst.w     etemp(%a6)       
         blt.b     res_neg          
res_pos:                            
         move.w    %d1,etemp(%a6)   # result in ETEMP
         bra.l     t_ovfl2          
res_neg:                            
         move.w    %d1,etemp(%a6)   # result in ETEMP
         lea.l     etemp(%a6),%a0   
         bra.l     t_unfl           
                                    
	version 3
