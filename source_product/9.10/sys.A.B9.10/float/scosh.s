ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/scosh.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:51:06 $
                                    
#
#	scosh.sa 3.1 12/10/90
#
#	The entry point sCosh computes the hyperbolic cosine of
#	an input argument; sCoshd does the same except for denormalized
#	input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value cosh(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program sCOSH takes approximately 250 cycles.
#
#	Algorithm:
#
#	COSH
#	1. If |X| > 16380 log2, go to 3.
#
#	2. (|X| <= 16380 log2) Cosh(X) is obtained by the formulae
#		y = |X|, z = exp(Y), and
#		cosh(X) = (1/2)*( z + 1/z ).
#		Exit.
#
#	3. (|X| > 16380 log2). If |X| > 16480 log2, go to 5.
#
#	4. (16380 log2 < |X| <= 16480 log2)
#		cosh(X) = sign(X) * exp(|X|)/2.
#		However, invoking exp(|X|) may cause premature overflow.
#		Thus, we calculate sinh(X) as follows:
#		Y	:= |X|
#		Fact	:=	2**(16380)
#		Y'	:= Y - 16381 log2
#		cosh(X) := Fact * exp(Y').
#		Exit.
#
#	5. (|X| > 16480 log2) sinh(X) must overflow. Return
#		Huge*Huge to generate overflow and an infinity with
#		the appropriate sign. Huge is the largest finite number in
#		extended format. Exit.
#
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
                                    
                                    
                                    
                                    
t1:      long      0x40c62d38,0xd3d64634 #  16381 LOG2 LEAD
t2:      long      0x3d6f90ae,0xb1e75cc7 #  16381 LOG2 TRAIL
                                    
two16380: long      0x7ffb0000,0x80000000,0x00000000,0x00000000 
                                    
         global    scoshd           
scoshd:                             
#--COSH(X) = 1 FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp0      
                                    
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 
         bra.l     t_frcinx         
                                    
         global    scosh            
scosh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x400cb167  
         bgt.b     coshbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 16380 LOG2
#--COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) )
                                    
         fabs.x    %fp0             # |X|
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       # pass parameter to setox
         bsr.l     setox            # FP0 IS EXP(|X|)
         fmul.s    &0f0.5,%fp0      # (1/2)EXP(|X|)
         move.l    (%sp)+,%d1       
                                    
         fmove.s   &0f0.25,%fp1     # (1/4)
         fdiv.x    %fp0,%fp1        # 1/(2 EXP(|X|))
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    %fp1,%fp0        
                                    
         bra.l     t_frcinx         
                                    
coshbig:                            
         cmpi.l    %d0,&0x400cb2b3  
         bgt.b     coshhuge         
                                    
         fabs.x    %fp0             
         fsub.d    t1(%pc),%fp0     # (|X|-16381LOG2_LEAD)
         fsub.d    t2(%pc),%fp0     # |X| - 16381 LOG2, ACCURATE
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            
         fmove.l   (%sp)+,%fpcr     
                                    
         fmul.x    two16380(%pc),%fp0 
         bra.l     t_frcinx         
                                    
coshhuge:                            
         fmove.l   &0,%fpsr         # clr N bit if set by source
         bclr.b    &7,(%a0)         # always return positive value
         fmovem.x  (%a0),%fp0       
         bra.l     t_ovfl           
                                    
                                    
	version 3
