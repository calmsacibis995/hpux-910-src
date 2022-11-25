ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/sint.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:51:49 $
                                    
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
#		number X in the ETEMP space in the floating-point
#		save stack.
#	       (Entry point sintdo) Double-extended number X in
#		location pointed to by the address register a0.
#	       (Entry point sintd) Double-extended denormalized
#		number X in the ETEMP space in the floating-point
#		save stack.
#
#	Output: The function returns int(X) or intrz(X) in fp0.
#
#	Modifies: fp0.
#
#	Algorithm: (sint and sintrz)
#
#	1. If exp(X) >= 63, return X. 
#	   If exp(X) < 0, return +/- 0 or +/- 1, according to
#	   the rounding mode.
#	
#	2. (X is in range) set rsc = 63 - exp(X). Unnormalize the
#	   result to the exponent $403e.
#
#	3. Round the result in the mode given in USER_FPCR. For
#	   sintrz, force round-to-zero mode.
#
#	4. Normalize the rounded result; store in fp0.
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
         global    sint             
sint:                               
         bfextu    fpcr_mode(%a6){&2:&2},%d1 # use user's mode for rounding
#					;implicity has extend precision
#					;in upper word. 
         move.l    %d1,l_scr1(%a6)  # save mode bits
         bra.b     sintexc          
                                    
#
#	FINT with extended denorm inputs.
#
         global    sintd            
sintd:                              
         btst.b    &5,fpcr_mode(%a6) 
         beq.l     snzrinx          # if round nearest or round zero, +/- 0
         btst.b    &4,fpcr_mode(%a6) 
         beq.b     rnd_mns          
rnd_pls:                            
         btst.b    &sign_bit,local_ex(%a0) 
         bne.b     sintmz           
         bsr.l     ld_pone          # if round plus inf and pos, answer is +1
         bra.l     t_inx2           
rnd_mns:                            
         btst.b    &sign_bit,local_ex(%a0) 
         beq.b     sintpz           
         bsr.l     ld_mone          # if round mns inf and neg, answer is -1
         bra.l     t_inx2           
sintpz:                             
         bsr.l     ld_pzero         
         bra.l     t_inx2           
sintmz:                             
         bsr.l     ld_mzero         
         bra.l     t_inx2           
                                    
#
#	FINTRZ
#
         global    sintrz           
sintrz:                             
         move.l    &1,l_scr1(%a6)   # use rz mode for rounding
#					;implicity has extend precision
#					;in upper word. 
         bra.b     sintexc          
#
#	SINTDO
#
#	Input:	a0 points to an IEEE extended format operand
# 	Output:	fp0 has the result 
#
# Exeptions:
#
# If the subroutine results in an inexact operation, the inx2 and
# ainx bits in the USER_FPSR are set.
#
#
         global    sintdo           
sintdo:                             
         bfextu    fpcr_mode(%a6){&2:&2},%d1 # use user's mode for rounding
#					;implicitly has ext precision
#					;in upper word. 
         move.l    %d1,l_scr1(%a6)  # save mode bits
#
# Real work of sint is in sintexc
#
sintexc:                            
         bclr.b    &sign_bit,local_ex(%a0) # convert to internal extended
#					;format
         sne       local_sgn(%a0)   
         cmp.w     local_ex(%a0),&0x403e # check if (unbiased) exp > 63
         bgt.b     out_rnge         # branch if exp < 63
         cmp.w     local_ex(%a0),&0x3ffd # check if (unbiased) exp < 0
         bgt.w     in_rnge          # if 63 >= exp > 0, do calc
#
# Input is less than zero.  Restore sign, and check for directed
# rounding modes.  L_SCR1 contains the rmode in the lower byte.
#
un_rnge:                            
         btst.b    &1,l_scr1+3(%a6) # check for rn and rz
         beq.b     un_rnrz          
         tst.b     local_sgn(%a0)   # check for sign
         bne.b     un_rmrp_neg      
#
# Sign is +.  If rp, load +1.0, if rm, load +0.0
#
         cmpi.b    l_scr1+3(%a6),&3 # check for rp
         beq.b     un_ldpone        # if rp, load +1.0
         bsr.l     ld_pzero         # if rm, load +0.0
         bra.l     t_inx2           
un_ldpone:                            
         bsr.l     ld_pone          
         bra.l     t_inx2           
#
# Sign is -.  If rm, load -1.0, if rp, load -0.0
#
un_rmrp_neg:                            
         cmpi.b    l_scr1+3(%a6),&2 # check for rm
         beq.b     un_ldmone        # if rm, load -1.0
         bsr.l     ld_mzero         # if rp, load -0.0
         bra.l     t_inx2           
un_ldmone:                            
         bsr.l     ld_mone          
         bra.l     t_inx2           
#
# Rmode is rn or rz; return signed zero
#
un_rnrz:                            
         tst.b     local_sgn(%a0)   # check for sign
         bne.b     un_rnrz_neg      
         bsr.l     ld_pzero         
         bra.l     t_inx2           
un_rnrz_neg:                            
         bsr.l     ld_mzero         
         bra.l     t_inx2           
                                    
#
# Input is greater than 2^63.  All bits are significant.  Return
# the input.
#
out_rnge:                            
         bfclr     local_sgn(%a0){&0:&8} # change back to IEEE ext format
         beq.b     intps            
         bset.b    &sign_bit,local_ex(%a0) 
intps:                              
         fmove.l   %fpcr,-(%sp)     
         fmove.l   &0,%fpcr         
         fmove.x   local_ex(%a0),%fp0 # if exp > 63
#					;then return X to the user
#					;there are no fraction bits
         fmove.l   (%sp)+,%fpcr     
         rts                        
                                    
in_rnge:                            
# 					;shift off fraction bits
         clr.l     %d0              # clear d0 - initial g,r,s for
#					;dnrm_lp
         move.l    &0x403e,%d1      # set threshold for dnrm_lp
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
#					;inexact flag set in USER_FPSR
#					;if initial grs set
#
# normalize the rounded result and store value in fp0
#
         bsr.l     nrm_set          # normalize the unnorm
#					;Input: a0 points to operand to
#					;be normalized
#					;Output: a0 points to normalized
#					;result
         bfclr     local_sgn(%a0){&0:&8} 
         beq.b     nrmrndp          
         bset.b    &sign_bit,local_ex(%a0) # return to IEEE extended format
nrmrndp:                            
         fmove.l   %fpcr,-(%sp)     
         fmove.l   &0,%fpcr         
         fmove.x   local_ex(%a0),%fp0 # move result to fp0
         fmove.l   (%sp)+,%fpcr     
         rts                        
                                    
                                    
	version 3
