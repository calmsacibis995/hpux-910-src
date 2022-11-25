ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/sasin.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:50:12 $
                                    
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
                                    
piby2:   long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
                                    
                                    
                                    
                                    
                                    
                                    
         global    sasind           
sasind:                             
#--ASIN(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
         global    sasin            
sasin:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
         cmpi.l    %d0,&0x3fff8000  
         bge.b     asinbig          
                                    
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
                                    
asinbig:                            
         fabs.x    %fp0             # |X|
         fcmp.s    %fp0,&0f1.0      
         fbgt.l    t_operr          # cause an operr exception
                                    
#--|X| = 1, ASIN(X) = +- PI/2.
                                    
         fmove.x   piby2,%fp0       
         move.l    (%a0),%d0        
         andi.l    &0x80000000,%d0  # SIGN BIT OF X
         ori.l     &0x3f800000,%d0  # +-1 IN SGL FORMAT
         move.l    %d0,-(%sp)       # push SIGN(X) IN SGL-FMT
         fmove.l   %d1,%fpcr        
         fmul.s    (%sp)+,%fp0      
         bra.l     t_frcinx         
                                    
                                    
	version 3
