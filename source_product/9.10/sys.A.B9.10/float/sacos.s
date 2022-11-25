ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/sacos.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:49:59 $
                                    
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
                                    
pi:      long      0x40000000,0xc90fdaa2,0x2168c235,0x00000000 
piby2:   long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
                                    
                                    
                                    
                                    
                                    
         global    sacosd           
sacosd:                             
#--ACOS(X) = PI/2 FOR DENORMALIZED X
         fmove.l   %d1,%fpcr        # load user's rounding mode/precision
         fmove.x   piby2,%fp0       
         bra.l     t_frcinx         
                                    
         global    sacos            
sacos:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        # pack exponent with upper 16 fraction
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x3fff8000  
         bge.b     acosbig          
                                    
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
                                    
acosbig:                            
         fabs.x    %fp0             
         fcmp.s    %fp0,&0f1.0      
         fbgt.l    t_operr          # cause an operr exception
                                    
#--|X| = 1, ACOS(X) = 0 OR PI
         move.l    (%a0),%d0        # pack exponent with upper 16 fraction
         move.w    4(%a0),%d0       
         cmp.l     %d0,&0           # D0 has original exponent+fraction
         bgt.b     acosp1           
                                    
#--X = -1
#Returns PI and inexact exception
         fmove.x   pi,%fp0          
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 # cause an inexact exception to be put
#					;into the 040 - will not trap until next
#					;fp inst.
         bra.l     t_frcinx         
                                    
acosp1:                             
         fmove.l   %d1,%fpcr        
         fmove.s   &0f0.0,%fp0      
         rts                        # Facos of +1 is exact	
                                    
                                    
	version 3
