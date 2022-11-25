ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/stwotox.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:54:29 $
                                    
#
#	stwotox.sa 3.1 12/10/90
#
#	stwotox  --- 2**X
#	stwotoxd --- 2**X for denormalized X
#	stentox  --- 10**X
#	stentoxd --- 10**X for denormalized X
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The function values are returned in Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 2 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program stwotox takes approximately 190 cycles and the
#		program stentox takes approximately 200 cycles.
#
#	Algorithm:
#
#	twotox
#	1. If |X| > 16480, go to ExpBig.
#
#	2. If |X| < 2**(-70), go to ExpSm.
#
#	3. Decompose X as X = N/64 + r where |r| <= 1/128. Furthermore
#		decompose N as
#		 N = 64(M + M') + j,  j = 0,1,2,...,63.
#
#	4. Overwrite r := r * log2. Then
#		2**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
#		Go to expr to compute that expression.
#
#	tentox
#	1. If |X| > 16480*log_10(2) (base 10 log of 2), go to ExpBig.
#
#	2. If |X| < 2**(-70), go to ExpSm.
#
#	3. Set y := X*log_2(10)*64 (base 2 log of 10). Set
#		N := round-to-int(y). Decompose N as
#		 N = 64(M + M') + j,  j = 0,1,2,...,63.
#
#	4. Define r as
#		r := ((X - N*L1)-N*L2) * L10
#		where L1, L2 are the leading and trailing parts of log_10(2)/64
#		and L10 is the natural log of 10. Then
#		10**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
#		Go to expr to compute that expression.
#
#	expr
#	1. Fetch 2**(j/64) from table as Fact1 and Fact2.
#
#	2. Overwrite Fact1 and Fact2 by
#		Fact1 := 2**(M) * Fact1
#		Fact2 := 2**(M) * Fact2
#		Thus Fact1 + Fact2 = 2**(M) * 2**(j/64).
#
#	3. Calculate P where 1 + P approximates exp(r):
#		P = r + r*r*(A1+r*(A2+...+r*A5)).
#
#	4. Let AdjFact := 2**(M'). Return
#		AdjFact * ( Fact1 + ((Fact1*P) + Fact2) ).
#		Exit.
#
#	ExpBig
#	1. Generate overflow by Huge * Huge if X > 0; otherwise, generate
#		underflow by Tiny * Tiny.
#
#	ExpSm
#	1. Return 1 + X.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
bounds1: long      0x3fb98000,0x400d80c0 #  2^(-70),16480
bounds2: long      0x3fb98000,0x400b9b07 #  2^(-70),16480 LOG2/LOG10
                                    
l2ten64: long      0x406a934f,0x0979a371 #  64LOG10/LOG2
l10two1: long      0x3f734413,0x509f8000 #  LOG2/64LOG10
                                    
l10two2: long      0xbfcd0000,0xc0219dc1,0xda994fd2,0x00000000 
                                    
log10:   long      0x40000000,0x935d8ddd,0xaaa8ac17,0x00000000 
                                    
log2:    long      0x3ffe0000,0xb17217f7,0xd1cf79ac,0x00000000 
                                    
expa5:   long      0x3f56c16d,0x6f7bd0b2 
expa4:   long      0x3f811112,0x302c712c 
expa3:   long      0x3fa55555,0x55554cc1 
expa2:   long      0x3fc55555,0x55554a54 
expa1:   long      0x3fe00000,0x00000000,0x00000000,0x00000000 
                                    
huge:    long      0x7ffe0000,0xffffffff,0xffffffff,0x00000000 
tiny:    long      0x00010000,0xffffffff,0xffffffff,0x00000000 
                                    
exptbl:                             
         long      0x3fff0000,0x80000000,0x00000000,0x3f738000 
         long      0x3fff0000,0x8164d1f3,0xbc030773,0x3fbef7ca 
         long      0x3fff0000,0x82cd8698,0xac2ba1d7,0x3fbdf8a9 
         long      0x3fff0000,0x843a28c3,0xacde4046,0x3fbcd7c9 
         long      0x3fff0000,0x85aac367,0xcc487b15,0xbfbde8da 
         long      0x3fff0000,0x871f6196,0x9e8d1010,0x3fbde85c 
         long      0x3fff0000,0x88980e80,0x92da8527,0x3fbebbf1 
         long      0x3fff0000,0x8a14d575,0x496efd9a,0x3fbb80ca 
         long      0x3fff0000,0x8b95c1e3,0xea8bd6e7,0xbfba8373 
         long      0x3fff0000,0x8d1adf5b,0x7e5ba9e6,0xbfbe9670 
         long      0x3fff0000,0x8ea4398b,0x45cd53c0,0x3fbdb700 
         long      0x3fff0000,0x9031dc43,0x1466b1dc,0x3fbeeeb0 
         long      0x3fff0000,0x91c3d373,0xab11c336,0x3fbbfd6d 
         long      0x3fff0000,0x935a2b2f,0x13e6e92c,0xbfbdb319 
         long      0x3fff0000,0x94f4efa8,0xfef70961,0x3fbdba2b 
         long      0x3fff0000,0x96942d37,0x20185a00,0x3fbe91d5 
         long      0x3fff0000,0x9837f051,0x8db8a96f,0x3fbe8d5a 
         long      0x3fff0000,0x99e04593,0x20b7fa65,0xbfbcde7b 
         long      0x3fff0000,0x9b8d39b9,0xd54e5539,0xbfbebaaf 
         long      0x3fff0000,0x9d3ed9a7,0x2cffb751,0xbfbd86da 
         long      0x3fff0000,0x9ef53260,0x91a111ae,0xbfbebedd 
         long      0x3fff0000,0xa0b0510f,0xb9714fc2,0x3fbcc96e 
         long      0x3fff0000,0xa2704303,0x0c496819,0xbfbec90b 
         long      0x3fff0000,0xa43515ae,0x09e6809e,0x3fbbd1db 
         long      0x3fff0000,0xa5fed6a9,0xb15138ea,0x3fbce5eb 
         long      0x3fff0000,0xa7cd93b4,0xe965356a,0xbfbec274 
         long      0x3fff0000,0xa9a15ab4,0xea7c0ef8,0x3fbea83c 
         long      0x3fff0000,0xab7a39b5,0xa93ed337,0x3fbecb00 
         long      0x3fff0000,0xad583eea,0x42a14ac6,0x3fbe9301 
         long      0x3fff0000,0xaf3b78ad,0x690a4375,0xbfbd8367 
         long      0x3fff0000,0xb123f581,0xd2ac2590,0xbfbef05f 
         long      0x3fff0000,0xb311c412,0xa9112489,0x3fbdfb3c 
         long      0x3fff0000,0xb504f333,0xf9de6484,0x3fbeb2fb 
         long      0x3fff0000,0xb6fd91e3,0x28d17791,0x3fbae2cb 
         long      0x3fff0000,0xb8fbaf47,0x62fb9ee9,0x3fbcdc3c 
         long      0x3fff0000,0xbaff5ab2,0x133e45fb,0x3fbee9aa 
         long      0x3fff0000,0xbd08a39f,0x580c36bf,0xbfbeaefd 
         long      0x3fff0000,0xbf1799b6,0x7a731083,0xbfbcbf51 
         long      0x3fff0000,0xc12c4cca,0x66709456,0x3fbef88a 
         long      0x3fff0000,0xc346ccda,0x24976407,0x3fbd83b2 
         long      0x3fff0000,0xc5672a11,0x5506dadd,0x3fbdf8ab 
         long      0x3fff0000,0xc78d74c8,0xabb9b15d,0xbfbdfb17 
         long      0x3fff0000,0xc9b9bd86,0x6e2f27a3,0xbfbefe3c 
         long      0x3fff0000,0xcbec14fe,0xf2727c5d,0xbfbbb6f8 
         long      0x3fff0000,0xce248c15,0x1f8480e4,0xbfbcee53 
         long      0x3fff0000,0xd06333da,0xef2b2595,0xbfbda4ae 
         long      0x3fff0000,0xd2a81d91,0xf12ae45a,0x3fbc9124 
         long      0x3fff0000,0xd4f35aab,0xcfedfa1f,0x3fbeb243 
         long      0x3fff0000,0xd744fcca,0xd69d6af4,0x3fbde69a 
         long      0x3fff0000,0xd99d15c2,0x78afd7b6,0xbfb8bc61 
         long      0x3fff0000,0xdbfbb797,0xdaf23755,0x3fbdf610 
         long      0x3fff0000,0xde60f482,0x5e0e9124,0xbfbd8be1 
         long      0x3fff0000,0xe0ccdeec,0x2a94e111,0x3fbacb12 
         long      0x3fff0000,0xe33f8972,0xbe8a5a51,0x3fbb9bfe 
         long      0x3fff0000,0xe5b906e7,0x7c8348a8,0x3fbcf2f4 
         long      0x3fff0000,0xe8396a50,0x3c4bdc68,0x3fbef22f 
         long      0x3fff0000,0xeac0c6e7,0xdd24392f,0xbfbdbf4a 
         long      0x3fff0000,0xed4f301e,0xd9942b84,0x3fbec01a 
         long      0x3fff0000,0xefe4b99b,0xdcdaf5cb,0x3fbe8cac 
         long      0x3fff0000,0xf281773c,0x59ffb13a,0xbfbcbb3f 
         long      0x3fff0000,0xf5257d15,0x2486cc2c,0x3fbef73a 
         long      0x3fff0000,0xf7d0df73,0x0ad13bb9,0xbfb8b795 
         long      0x3fff0000,0xfa83b2db,0x722a033a,0x3fbef84b 
         long      0x3fff0000,0xfd3e0c0c,0xf486c175,0xbfbef581 
                                    
         set       n,l_scr1         
                                    
         set       x,fp_scr1        
         set       xdcare,x+2       
         set       xfrac,x+4        
                                    
         set       adjfact,fp_scr2  
                                    
         set       fact1,fp_scr3    
         set       fact1hi,fact1+4  
         set       fact1low,fact1+8 
                                    
         set       fact2,fp_scr4    
         set       fact2hi,fact2+4  
         set       fact2low,fact2+8 
                                    
                                    
                                    
                                    
                                    
         global    stwotoxd         
stwotoxd:                            
#--ENTRY POINT FOR 2**(X) FOR DENORMALIZED ARGUMENT
                                    
         fmove.l   %d1,%fpcr        # set user's rounding mode/precision
         fmove.s   &0f1.0,%fp0      # RETURN 1 + X
         move.l    (%a0),%d0        
         or.l      &0x00800001,%d0  
         fadd.s    %d0,%fp0         
         bra.l     t_frcinx         
                                    
         global    stwotox          
stwotox:                            
#--ENTRY POINT FOR 2**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
         fmovem.x  (%a0),%fp0       # LOAD INPUT, do not set cc's
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,x(%a6)      
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3fb98000  # |X| >= 2**(-70)?
         bge.b     twook1           
         bra.w     expbors          
                                    
twook1:                             
         cmpi.l    %d0,&0x400d80c0  # |X| > 16480?
         ble.b     twomain          
         bra.w     expbors          
                                    
                                    
twomain:                            
#--USUAL CASE, 2^(-70) <= |X| <= 16480
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0f64.0,%fp1     # 64 * X
                                    
         fmove.l   %fp1,n(%a6)      # N = ROUND-TO-INT(64 X)
         move.l    %d2,-(%sp)       
         lea       exptbl,%a1       # LOAD ADDRESS OF TABLE OF 2^(J/64)
         fmove.l   n(%a6),%fp1      # N --> FLOATING FMT
         move.l    n(%a6),%d0       
         move.l    %d0,%d2          
         andi.l    &0x3f,%d0        # D0 IS J
         asl.l     &4,%d0           # DISPLACEMENT FOR 2^(J/64)
         adda.l    %d0,%a1          # ADDRESS FOR 2^(J/64)
         asr.l     &6,%d2           # d2 IS L, N = 64L + J
         move.l    %d2,%d0          
         asr.l     &1,%d0           # D0 IS M
         sub.l     %d0,%d2          # d2 IS M__hpasm_string1) + J
         addi.l    &0x3fff,%d2      
         move.w    %d2,adjfact(%a6) # ADJFACT IS 2^(M')
         move.l    (%sp)+,%d2       
#--SUMMARY: a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
#--D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
#--ADJFACT = 2^(M').
#--REGISTERS SAVED SO FAR ARE (IN ORDER) FPCR, D0, FP1, a1, AND FP2.
                                    
         fmul.s    &0f0.015625,%fp1 # (1/64)*N
         move.l    (%a1)+,fact1(%a6) 
         move.l    (%a1)+,fact1hi(%a6) 
         move.l    (%a1)+,fact1low(%a6) 
         move.w    (%a1)+,fact2(%a6) 
         clr.w     fact2+2(%a6)     
                                    
         fsub.x    %fp1,%fp0        # X - (1/64)*INT(64 X)
                                    
         move.w    (%a1)+,fact2hi(%a6) 
         clr.w     fact2hi+2(%a6)   
         clr.l     fact2low(%a6)    
         add.w     %d0,fact1(%a6)   
                                    
         fmul.x    log2,%fp0        # FP0 IS R
         add.w     %d0,fact2(%a6)   
                                    
         bra.w     expr             
                                    
expbors:                            
#--FPCR, D0 SAVED
         cmpi.l    %d0,&0x3fff8000  
         bgt.b     expbig           
                                    
expsm:                              
#--|X| IS SMALL, RETURN 1 + X
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.s    &0f1.0,%fp0      # RETURN 1 + X
                                    
         bra.l     t_frcinx         
                                    
expbig:                             
#--|X| IS LARGE, GENERATE OVERFLOW IF X > 0; ELSE GENERATE UNDERFLOW
#--REGISTERS SAVE SO FAR ARE FPCR AND  D0
         move.l    x(%a6),%d0       
         cmpi.l    %d0,&0           
         blt.b     expneg           
                                    
         bclr.b    &7,(%a0)         # t_ovfl expects positive value
         bra.l     t_ovfl           
                                    
expneg:                             
         bclr.b    &7,(%a0)         # t_unfl expects positive value
         bra.l     t_unfl           
                                    
         global    stentoxd         
stentoxd:                            
#--ENTRY POINT FOR 10**(X) FOR DENORMALIZED ARGUMENT
                                    
         fmove.l   %d1,%fpcr        # set user's rounding mode/precision
         fmove.s   &0f1.0,%fp0      # RETURN 1 + X
         move.l    (%a0),%d0        
         or.l      &0x00800001,%d0  
         fadd.s    %d0,%fp0         
         bra.l     t_frcinx         
                                    
         global    stentox          
stentox:                            
#--ENTRY POINT FOR 10**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
         fmovem.x  (%a0),%fp0       # LOAD INPUT, do not set cc's
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,x(%a6)      
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3fb98000  # |X| >= 2**(-70)?
         bge.b     tenok1           
         bra.w     expbors          
                                    
tenok1:                             
         cmpi.l    %d0,&0x400b9b07  # |X| <= 16480*log2/log10 ?
         ble.b     tenmain          
         bra.w     expbors          
                                    
tenmain:                            
#--USUAL CASE, 2^(-70) <= |X| <= 16480 LOG 2 / LOG 10
                                    
         fmove.x   %fp0,%fp1        
         fmul.d    l2ten64,%fp1     # X*64*LOG10/LOG2
                                    
         fmove.l   %fp1,n(%a6)      # N=INT(X*64*LOG10/LOG2)
         move.l    %d2,-(%sp)       
         lea       exptbl,%a1       # LOAD ADDRESS OF TABLE OF 2^(J/64)
         fmove.l   n(%a6),%fp1      # N --> FLOATING FMT
         move.l    n(%a6),%d0       
         move.l    %d0,%d2          
         andi.l    &0x3f,%d0        # D0 IS J
         asl.l     &4,%d0           # DISPLACEMENT FOR 2^(J/64)
         adda.l    %d0,%a1          # ADDRESS FOR 2^(J/64)
         asr.l     &6,%d2           # d2 IS L, N = 64L + J
         move.l    %d2,%d0          
         asr.l     &1,%d0           # D0 IS M
         sub.l     %d0,%d2          # d2 IS M__hpasm_string1) + J
         addi.l    &0x3fff,%d2      
         move.w    %d2,adjfact(%a6) # ADJFACT IS 2^(M')
         move.l    (%sp)+,%d2       
                                    
#--SUMMARY: a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
#--D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
#--ADJFACT = 2^(M').
#--REGISTERS SAVED SO FAR ARE (IN ORDER) FPCR, D0, FP1, a1, AND FP2.
                                    
         fmove.x   %fp1,%fp2        
                                    
         fmul.d    l10two1,%fp1     # N*(LOG2/64LOG10)_LEAD
         move.l    (%a1)+,fact1(%a6) 
                                    
         fmul.x    l10two2,%fp2     # N*(LOG2/64LOG10)_TRAIL
                                    
         move.l    (%a1)+,fact1hi(%a6) 
         move.l    (%a1)+,fact1low(%a6) 
         fsub.x    %fp1,%fp0        # X - N L_LEAD
         move.w    (%a1)+,fact2(%a6) 
                                    
         fsub.x    %fp2,%fp0        # X - N L_TRAIL
                                    
         clr.w     fact2+2(%a6)     
         move.w    (%a1)+,fact2hi(%a6) 
         clr.w     fact2hi+2(%a6)   
         clr.l     fact2low(%a6)    
                                    
         fmul.x    log10,%fp0       # FP0 IS R
                                    
         add.w     %d0,fact1(%a6)   
         add.w     %d0,fact2(%a6)   
                                    
expr:                               
#--FPCR, FP2, FP3 ARE SAVED IN ORDER AS SHOWN.
#--ADJFACT CONTAINS 2**(M'), FACT1 + FACT2 = 2**(M) * 2**(J/64).
#--FP0 IS R. THE FOLLOWING CODE COMPUTES
#--	2**(M'+M) * 2**(J/64) * EXP(R)
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS S = R*R
                                    
         fmove.d   expa5,%fp2       # FP2 IS A5
         fmove.d   expa4,%fp3       # FP3 IS A4
                                    
         fmul.x    %fp1,%fp2        # FP2 IS S*A5
         fmul.x    %fp1,%fp3        # FP3 IS S*A4
                                    
         fadd.d    expa3,%fp2       # FP2 IS A3+S*A5
         fadd.d    expa2,%fp3       # FP3 IS A2+S*A4
                                    
         fmul.x    %fp1,%fp2        # FP2 IS S*(A3+S*A5)
         fmul.x    %fp1,%fp3        # FP3 IS S*(A2+S*A4)
                                    
         fadd.d    expa1,%fp2       # FP2 IS A1+S*(A3+S*A5)
         fmul.x    %fp0,%fp3        # FP3 IS R*S*(A2+S*A4)
                                    
         fmul.x    %fp1,%fp2        # FP2 IS S*(A1+S*(A3+S*A5))
         fadd.x    %fp3,%fp0        # FP0 IS R+R*S*(A2+S*A4)
                                    
         fadd.x    %fp2,%fp0        # FP0 IS EXP(R) - 1
                                    
                                    
#--FINAL RECONSTRUCTION PROCESS
#--EXP(X) = 2^M*2^(J/64) + 2^M*2^(J/64)*(EXP(R)-1)  -  (1 OR 0)
                                    
         fmul.x    fact1(%a6),%fp0  
         fadd.x    fact2(%a6),%fp0  
         fadd.x    fact1(%a6),%fp0  
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         clr.w     adjfact+2(%a6)   
         move.l    &0x80000000,adjfact+4(%a6) 
         clr.l     adjfact+8(%a6)   
         fmul.x    adjfact(%a6),%fp0 # FINAL ADJUSTMENT
                                    
         bra.l     t_frcinx         
                                    
                                    
	version 3
