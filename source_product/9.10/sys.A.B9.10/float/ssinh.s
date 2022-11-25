ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/ssinh.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:53:35 $
                                    
#
#	ssinh.sa 3.1 12/10/90
#
#       The entry point sSinh computes the hyperbolic sine of
#       an input argument; sSinhd does the same except for denormalized
#       input.
#
#       Input: Double-extended number X in location pointed to 
#		by address register a0.
#
#       Output: The value sinh(X) returned in floating-point register Fp0.
#
#       Accuracy and Monotonicity: The returned result is within 3 ulps in
#               64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#               result is subsequently rounded to double precision. The
#               result is provably monotonic in double precision.
#
#       Speed: The program sSINH takes approximately 280 cycles.
#
#       Algorithm:
#
#       SINH
#       1. If |X| > 16380 log2, go to 3.
#
#       2. (|X| <= 16380 log2) Sinh(X) is obtained by the formulae
#               y = |X|, sgn = sign(X), and z = expm1(Y),
#               sinh(X) = sgn*(1/2)*( z + z/(1+z) ).
#          Exit.
#
#       3. If |X| > 16480 log2, go to 5.
#
#       4. (16380 log2 < |X| <= 16480 log2)
#               sinh(X) = sign(X) * exp(|X|)/2.
#          However, invoking exp(|X|) may cause premature overflow.
#          Thus, we calculate sinh(X) as follows:
#             Y       := |X|
#             sgn     := sign(X)
#             sgnFact := sgn * 2**(16380)
#             Y'      := Y - 16381 log2
#             sinh(X) := sgnFact * exp(Y').
#          Exit.
#
#       5. (|X| > 16480 log2) sinh(X) must overflow. Return
#          sign(X)*Huge*Huge to generate overflow and an infinity with
#          the appropriate sign. Huge is the largest finite number in
#          extended format. Exit.
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
                                    
                                    
                                    
                                    
                                    
                                    
                                    
         global    ssinhd           
ssinhd:                             
#--SINH(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
         global    ssinh            
ssinh:                              
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         move.l    %d0,%a1          
         and.l     &0x7fffffff,%d0  
         cmp.l     %d0,&0x400cb167  
         bgt.b     sinhbig          
                                    
#--THIS IS THE USUAL CASE, |X| < 16380 LOG2
#--Y = |X|, Z = EXPM1(Y), SINH(X) = SIGN(X)*(1/2)*( Z + Z/(1+Z) )
                                    
         fabs.x    %fp0             # Y = |X|
                                    
         movem.l   %a1/%d1,-(%sp)   
         fmovem.x  %fp0,(%a0)       
         clr.l     %d1              
         bsr.l     setoxm1          # FP0 IS Z = EXPM1(Y)
         fmove.l   &0,%fpcr         
         movem.l   (%sp)+,%a1/%d1   
                                    
         fmove.x   %fp0,%fp1        
         fadd.s    &0f1.0,%fp1      # 1+Z
         fmove.x   %fp0,-(%sp)      
         fdiv.x    %fp1,%fp0        # Z/(1+Z)
         move.l    %a1,%d0          
         and.l     &0x80000000,%d0  
         or.l      &0x3f000000,%d0  
         fadd.x    (%sp)+,%fp0      
         move.l    %d0,-(%sp)       
                                    
         fmove.l   %d1,%fpcr        
         fmul.s    (%sp)+,%fp0      # last fp inst - possible exceptions set
                                    
         bra.l     t_frcinx         
                                    
sinhbig:                            
         cmp.l     %d0,&0x400cb2b3  
         bgt.l     t_ovfl           
         fabs.x    %fp0             
         fsub.d    t1(%pc),%fp0     # (|X|-16381LOG2_LEAD)
         move.l    &0,-(%sp)        
         move.l    &0x80000000,-(%sp) 
         move.l    %a1,%d0          
         and.l     &0x80000000,%d0  
         or.l      &0x7ffb0000,%d0  
         move.l    %d0,-(%sp)       # EXTENDED FMT
         fsub.d    t2(%pc),%fp0     # |X| - 16381 LOG2, ACCURATE
                                    
         move.l    %d1,-(%sp)       
         clr.l     %d1              
         fmovem.x  %fp0,(%a0)       
         bsr.l     setox            
         fmove.l   (%sp)+,%fpcr     
                                    
         fmul.x    (%sp)+,%fp0      # possible exception
         bra.l     t_frcinx         
                                    
                                    
	version 3
