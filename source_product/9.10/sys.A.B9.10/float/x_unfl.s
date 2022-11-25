ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_unfl.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:56:51 $
                                    
#
#	x_unfl.sa 3.4 7/1/91
#
#	fpsp_unfl --- FPSP handler for underflow exception
#
# Trap disabled results
#	For 881/2 compatibility, sw must denormalize the intermediate 
# result, then store the result.  Denormalization is accomplished 
# by taking the intermediate result (which is always normalized) and 
# shifting the mantissa right while incrementing the exponent until 
# it is equal to the denormalized exponent for the destination 
# format.  After denormalizatoin, the result is rounded to the 
# destination format.
#		
# Trap enabled results
# 	All trap disabled code applies.	In addition the exceptional 
# operand needs to made available to the user with a bias of $6000 
# added to the exponent.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_unfl        
fpsp_unfl:                            
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
#
         bsr.l     unf_res          # denormalize, round & store interm op
#
# If underflow exceptions are not enabled, check for inexact
# exception
#
         btst.b    &unfl_bit,fpcr_enable(%a6) 
         beq.b     ck_inex          
                                    
         btst.b    &e3,e_byte(%a6)  
         beq.b     no_e3_1          
#
# Clear dirty bit on dest resister in the frame before branching
# to b1238_fix.
#
         bfextu    cmdreg3b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        # test for bug1238 case
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
no_e3_1:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_unfl        
#
# It is possible to have either inex2 or inex1 exceptions with the
# unfl.  If the inex enable bit is set in the FPCR, and either
# inex2 or inex1 occured, we must clean up and branch to the
# real inex handler.
#
ck_inex:                            
         move.b    fpcr_enable(%a6),%d0 
         and.b     fpsr_except(%a6),%d0 
         andi.b    &0x3,%d0         
         beq.b     unfl_done        
                                    
#
# Inexact enabled and reported, and we must take an inexact exception
#	
take_inex:                            
         btst.b    &e3,e_byte(%a6)  
         beq.b     no_e3_2          
#
# Clear dirty bit on dest resister in the frame before branching
# to b1238_fix.
#
         bfextu    cmdreg3b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        # test for bug1238 case
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
no_e3_2:                            
         move.b    &inex_vec,exc_vec+1(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_inex        
                                    
unfl_done:                            
         bclr.b    &e3,e_byte(%a6)  
         beq.b     e1_set           # if set then branch
#
# Clear dirty bit on dest resister in the frame before branching
# to b1238_fix.
#
         bfextu    cmdreg3b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        # test for bug1238 case
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     fpsp_done        
e1_set:                             
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         unlk      %a6              
         bra.l     fpsp_done        
#
#	unf_res --- underflow result calculation
#
unf_res:                            
         bsr.l     g_rndpr          # returns RND_PREC in d0 0=ext,
#					;1=sgl, 2=dbl
#					;we need the RND_PREC in the
#					;upper word for round
         move.w    &0,-(%a7)        
         move.w    %d0,-(%a7)       # copy RND_PREC to stack
#
#
# If the exception bit set is E3, the exceptional operand from the
# fpu is in WBTEMP; else it is in FPTEMP.
#
         btst.b    &e3,e_byte(%a6)  
         beq.b     unf_e1           
unf_e3:                             
         lea       wbtemp(%a6),%a0  # a0 now points to operand
#
# Test for fsgldiv and fsglmul.  If the inst was one of these, then
# force the precision to extended for the denorm routine.  Use
# the user's precision for the round routine.
#
         move.w    cmdreg3b(%a6),%d1 # check for fsgldiv or fsglmul
         andi.w    &0x7f,%d1        
         cmpi.w    %d1,&0x30        # check for sgldiv
         beq.b     unf_sgl          
         cmpi.w    %d1,&0x33        # check for sglmul
         bne.b     unf_cont         # if not, use fpcr prec in round
unf_sgl:                            
         clr.l     %d0              
         move.w    &0x1,(%a7)       # override g_rndpr precision
#					;force single
         bra.b     unf_cont         
unf_e1:                             
         lea       fptemp(%a6),%a0  # a0 now points to operand
unf_cont:                            
         bclr.b    &sign_bit,local_ex(%a0) # clear sign bit
         sne       local_sgn(%a0)   # store sign
                                    
         bsr.l     denorm           # returns denorm, a0 points to it
#
# WARNING:
#				;d0 has guard,round sticky bit
#				;make sure that it is not corrupted
#				;before it reaches the round subroutine
#				;also ensure that a0 isn't corrupted
                                    
#
# Set up d1 for round subroutine d1 contains the PREC/MODE
# information respectively on upper/lower register halves.
#
         bfextu    fpcr_mode(%a6){&2:&2},%d1 # get mode from FPCR
#						;mode in lower d1
         add.l     (%a7)+,%d1       # merge PREC/MODE
#
# WARNING: a0 and d0 are assumed to be intact between the denorm and
# round subroutines. All code between these two subroutines
# must not corrupt a0 and d0.
#
#
# Perform Round	
#	Input:		a0 points to input operand
#			d0{31:29} has guard, round, sticky
#			d1{01:00} has rounding mode
#			d1{17:16} has rounding precision
#	Output:		a0 points to rounded operand
#
                                    
         bsr.l     round            # returns rounded denorm at (a0)
#
# Differentiate between store to memory vs. store to register
#
unf_store:                            
         bsr.l     g_opcls          # returns opclass in d0{2:0}
         cmpi.b    %d0,&0x3         
         bne.b     not_opc011       
#
# At this point, a store to memory is pending
#
opc011:                             
         bsr.l     g_dfmtou         
         tst.b     %d0              
         beq.b     ext_opc011       # If extended, do not subtract
# 				;If destination format is sgl/dbl, 
         tst.b     local_hi(%a0)    # If rounded result is normal,don't
#					;subtract
         bmi.b     ext_opc011       
         subq.w    &1,local_ex(%a0) # account for denorm bias vs.
#				;normalized bias
#				;          normalized   denormalized
#				;single       $7f           $7e
#				;double       $3ff          $3fe
#
ext_opc011:                            
         bsr.l     store            # stores to memory
         bra.b     unf_done         # finish up
                                    
#
# At this point, a store to a float register is pending
#
not_opc011:                            
         bsr.l     store            # stores to float register
#				;a0 is not corrupted on a store to a
#				;float register.
#
# Set the condition codes according to result
#
         tst.l     local_hi(%a0)    # check upper mantissa
         bne.b     ck_sgn           
         tst.l     local_lo(%a0)    # check lower mantissa
         bne.b     ck_sgn           
         bset.b    &z_bit,fpsr_cc(%a6) # set condition codes if zero
ck_sgn:                             
         btst.b    &sign_bit,local_ex(%a0) # check the sign bit
         beq.b     unf_done         
         bset.b    &neg_bit,fpsr_cc(%a6) 
                                    
#
# Finish.  
#
unf_done:                            
         btst.b    &inex2_bit,fpsr_except(%a6) 
         beq.b     no_aunfl         
         bset.b    &aunfl_bit,fpsr_aexcept(%a6) 
no_aunfl:                            
         rts                        
                                    
                                    
	version 3
