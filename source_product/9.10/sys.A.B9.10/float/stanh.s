ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/stanh.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:54:03 $
                                    
#
#	stanh.sa 3.1 12/10/90
#
#	The entry point sTanh computes the hyperbolic tangent of
#	an input argument; sTanhd does the same except for denormalized
#	input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value tanh(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program stanh takes approximately 270 cycles.
#
#	Algorithm:
#
#	TANH
#	1. If |X| >= (5/2) log2 or |X| <= 2**(-40), go to 3.
#
#	2. (2**(-40) < |X| < (5/2) log2) Calculate tanh(X) by
#		sgn := sign(X), y := 2|X|, z := expm1(Y), and
#		tanh(X) = sgn*( z/(2+z) ).
#		Exit.
#
#	3. (|X| <= 2**(-40) or |X| >= (5/2) log2). If |X| < 1,
#		go to 7.
#
#	4. (|X| >= (5/2) log2) If |X| >= 50 log2, go to 6.
#
#	5. ((5/2) log2 <= |X| < 50 log2) Calculate tanh(X) by
#		sgn := sign(X), y := 2|X|, z := exp(Y),
#		tanh(X) = sgn - [ sgn*2/(1+z) ].
#		Exit.
#
#	6. (|X| >= 50 log2) Tanh(X) = +-1 (round to nearest). Thus, we
#		calculate Tanh(X) by
#		sgn := sign(X), Tiny := 2**(-126),
#		tanh(X) := sgn - sgn*Tiny.
#		Exit.
#
#	7. (|X| < 2**(-40)). Tanh(X) = X.	Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
         set       x,fp_scr5        
         set       xdcare,x+2       
         set       xfrac,x+4        
                                    
         set       sgn,l_scr3       
                                    
         set       v,fp_scr6        
                                    
bounds1: long      0x3fd78000,0x3fffddce #  2^(-40), (5/2)LOG2
                                    
                                    
                                    
                                    
                                    
                                    
         global    stanhd           
stanhd:                             
#--TANH(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
         global    stanh            
stanh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         fmove.x   %fp0,x(%a6)      
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         move.l    %d0,x(%a6)       
         and.l     &0x7fffffff,%d0  
         cmp2.l    %d0,bounds1(%pc) # 2**(-40) < |X| < (5/2)LOG2 ?
         bcs.b     tanhbors         
                                    
#--THIS IS THE USUAL CASE
#--Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2).
                                    
         move.l    x(%a6),%d0       
         move.l    %d0,sgn(%a6)     
         and.l     &0x7fff0000,%d0  
         add.l     &0x00010000,%d0  # EXPONENT OF 2|X|
         move.l    %d0,x(%a6)       
         and.l     &0x80000000,sgn(%a6) 
         fmove.x   x(%a6),%fp0      # FP0 IS Y = 2|X|
                                    
         move.l    %d1,-(%a7)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setoxm1          # FP0 IS Z = EXPM1(Y)
         move.l    (%a7)+,%d1       
                                    
         fmove.x   %fp0,%fp1        
         fadd.s    &0f2.0,%fp1      # Z+2
         move.l    sgn(%a6),%d0     
         fmove.x   %fp1,v(%a6)      
         eor.l     %d0,v(%a6)       
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fdiv.x    v(%a6),%fp0      
         bra.l     t_frcinx         
                                    
tanhbors:                            
         cmp.l     %d0,&0x3fff8000  
         blt.w     tanhsm           
                                    
         cmp.l     %d0,&0x40048aa1  
         bgt.w     tanhhuge         
                                    
#-- (5/2) LOG2 < |X| < 50 LOG2,
#--TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X),
#--TANH(X) = SGN -	SGN*2/[EXP(Y)+1].
                                    
         move.l    x(%a6),%d0       
         move.l    %d0,sgn(%a6)     
         and.l     &0x7fff0000,%d0  
         add.l     &0x00010000,%d0  # EXPO OF 2|X|
         move.l    %d0,x(%a6)       # Y = 2|X|
         and.l     &0x80000000,sgn(%a6) 
         move.l    sgn(%a6),%d0     
         fmove.x   x(%a6),%fp0      # Y = 2|X|
                                    
         move.l    %d1,-(%a7)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            # FP0 IS EXP(Y)
         move.l    (%a7)+,%d1       
         move.l    sgn(%a6),%d0     
         fadd.s    &0f1.0,%fp0      # EXP(Y)+1
                                    
         eor.l     &0xc0000000,%d0  # -SIGN(X)*2
         fmove.s   %d0,%fp1         # -SIGN(X)*2 IN SGL FMT
         fdiv.x    %fp0,%fp1        # -SIGN(X)2 / [EXP(Y)+1 ]
                                    
         move.l    sgn(%a6),%d0     
         or.l      &0x3f800000,%d0  # SGN
         fmove.s   %d0,%fp0         # SGN IN SGL FMT
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    %fp1,%fp0        
                                    
         bra.l     t_frcinx         
                                    
tanhsm:                             
         move.w    &0x0000,xdcare(%a6) 
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   x(%a6),%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
tanhhuge:                            
#---RETURN SGN(X) - SGN(X)EPS
         move.l    x(%a6),%d0       
         and.l     &0x80000000,%d0  
         or.l      &0x3f800000,%d0  
         fmove.s   %d0,%fp0         
         and.l     &0x80000000,%d0  
         eor.l     &0x80800000,%d0  # -SIGN(X)*EPS
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.s    %d0,%fp0         
                                    
         bra.l     t_frcinx         
                                    
                                    
	version 3
