ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/x_ovfl.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:56:02 $
                                    
#
#	x_ovfl.sa 3.5 7/1/91
#
#	fpsp_ovfl --- FPSP handler for overflow exception
#
#	Overflow occurs when a floating-point intermediate result is
#	too large to be represented in a floating-point data register,
#	or when storing to memory, the contents of a floating-point
#	data register are too large to be represented in the
#	destination format.
#		
# Trap disabled results
#
# If the instruction is move_out, then garbage is stored in the
# destination.  If the instruction is not move_out, then the
# destination is not affected.  For 68881 compatibility, the
# following values should be stored at the destination, based
# on the current rounding mode:
#
#  RN	Infinity with the sign of the intermediate result.
#  RZ	Largest magnitude number, with the sign of the
#	intermediate result.
#  RM   For pos overflow, the largest pos number. For neg overflow,
#	-infinity
#  RP   For pos overflow, +infinity. For neg overflow, the largest
#	neg number
#
# Trap enabled results
# All trap disabled code applies.  In addition the exceptional
# operand needs to be made available to the users exception handler
# with a bias of $6000 subtracted from the exponent.
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
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    fpsp_ovfl        
fpsp_ovfl:                            
         link      %a6,&-local_size 
         fsave     -(%a7)           
         movem.l   %d0-%d1/%a0-%a1,user_da(%a6) 
         fmovem.x  %fp0-%fp3,user_fp0(%a6) 
         fmovem.l  %fpcr/%fpsr/%fpiar,user_fpcr(%a6) 
                                    
#
#	The 040 doesn't set the AINEX bit in the FPSR, the following
#	line temporarily rectifies this error.
#
         bset.b    &ainex_bit,fpsr_aexcept(%a6) 
#
         bsr.l     ovf_adj          # denormalize, round & store interm op
#
#	if overflow traps not enabled check for inexact exception
#
         btst.b    &ovfl_bit,fpcr_enable(%a6) 
         beq.b     ck_inex          
#
         btst.b    &e3,e_byte(%a6)  
         beq.b     no_e3_1          
         bfextu    cmdreg3b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        
         move.l    user_fpsr(%a6),fpsr_shadow(%a6) 
         or.l      &sx_mask,e_byte(%a6) 
no_e3_1:                            
         movem.l   user_da(%a6),%d0-%d1/%a0-%a1 
         fmovem.x  user_fp0(%a6),%fp0-%fp3 
         fmovem.l  user_fpcr(%a6),%fpcr/%fpsr/%fpiar 
         frestore  (%a7)+           
         unlk      %a6              
         bra.l     real_ovfl        
#
# It is possible to have either inex2 or inex1 exceptions with the
# ovfl.  If the inex enable bit is set in the FPCR, and either
# inex2 or inex1 occured, we must clean up and branch to the
# real inex handler.
#
ck_inex:                            
#	move.b		FPCR_ENABLE(a6),d0
#	and.b		FPSR_EXCEPT(a6),d0
#	andi.b		#$3,d0
         btst.b    &inex2_bit,fpcr_enable(%a6) 
         beq.b     ovfl_exit        
#
# Inexact enabled and reported, and we must take an inexact exception.
#
take_inex:                            
         btst.b    &e3,e_byte(%a6)  
         beq.b     no_e3_2          
         bfextu    cmdreg3b(%a6){&6:&3},%d0 # get dest reg no
         bclr.b    %d0,fpr_dirty_bits(%a6) # clr dest dirty bit
         bsr.l     b1238_fix        
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
                                    
ovfl_exit:                            
         bclr.b    &e3,e_byte(%a6)  # test and clear E3 bit
         beq.b     e1_set           
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
#	ovf_adj
#
ovf_adj:                            
#
# Have a0 point to the correct operand. 
#
         btst.b    &e3,e_byte(%a6)  # test E3 bit
         beq.b     ovf_e1           
                                    
         lea       wbtemp(%a6),%a0  
         bra.b     ovf_com          
ovf_e1:                             
         lea       etemp(%a6),%a0   
                                    
ovf_com:                            
         bclr.b    &sign_bit,local_ex(%a0) 
         sne       local_sgn(%a0)   
                                    
         bsr.l     g_opcls          # returns opclass in d0
         cmpi.w    %d0,&3           # check for opclass3
         bne.b     not_opc011       
                                    
#
# FPSR_CC is saved and restored because ovf_r_x3 affects it. The
# CCs are defined to be 'not affected' for the opclass3 instruction.
#
         move.b    fpsr_cc(%a6),l_scr1(%a6) 
         bsr.l     ovf_r_x3         # returns a0 pointing to result
         move.b    l_scr1(%a6),fpsr_cc(%a6) 
         bra.l     store            # stores to memory or register
                                    
not_opc011:                            
         bsr.l     ovf_r_x2         # returns a0 pointing to result
         bra.l     store            # stores to memory or register
                                    
                                    
	version 3
