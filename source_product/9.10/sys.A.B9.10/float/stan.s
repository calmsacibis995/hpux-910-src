ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/stan.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:53:49 $
                                    
#
#	stan.sa 3.3 7/29/91
#
#	The entry point stan computes the tangent of
#	an input argument;
#	stand does the same except for denormalized input.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The value tan(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 3 ulp in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program sTAN takes approximately 170 cycles for
#		input argument X such that |X| < 15Pi, which is the the usual
#		situation.
#
#	Algorithm:
#
#	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.
#
#	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 2, so in particular, k = 0 or 1.
#
#	3. If k is odd, go to 5.
#
#	4. (k is even) Tan(X) = tan(r) and tan(r) is approximated by a
#		rational function U/V where
#		U = r + r*s*(P1 + s*(P2 + s*P3)), and
#		V = 1 + s*(Q1 + s*(Q2 + s*(Q3 + s*Q4))),  s = r*r.
#		Exit.
#
#	4. (k is odd) Tan(X) = -cot(r). Since tan(r) is approximated by a
#		rational function U/V where
#		U = r + r*s*(P1 + s*(P2 + s*P3)), and
#		V = 1 + s*(Q1 + s*(Q2 + s*(Q3 + s*Q4))), s = r*r,
#		-Cot(r) = -V/U. Exit.
#
#	6. If |X| > 1, go to 8.
#
#	7. (|X|<2**(-40)) Tan(X) = X. Exit.
#
#	8. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 2.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
bounds1: long      0x3fd78000,0x4004bc7e 
twobypi: long      0x3fe45f30,0x6dc9c883 
                                    
tanq4:   long      0x3ea0b759,0xf50f8688 
tanp3:   long      0xbef2baa5,0xa8924f04 
                                    
tanq3:   long      0xbf346f59,0xb39ba65f,0x00000000,0x00000000 
                                    
tanp2:   long      0x3ff60000,0xe073d3fc,0x199c4a00,0x00000000 
                                    
tanq2:   long      0x3ff90000,0xd23cd684,0x15d95fa1,0x00000000 
                                    
tanp1:   long      0xbffc0000,0x8895a6c5,0xfb423bca,0x00000000 
                                    
tanq1:   long      0xbffd0000,0xeef57e0d,0xa84bc8ce,0x00000000 
                                    
invtwopi: long      0x3ffc0000,0xa2f9836e,0x4e44152a,0x00000000 
                                    
twopi1:  long      0x40010000,0xc90fdaa2,0x00000000,0x00000000 
twopi2:  long      0x3fdf0000,0x85a308d4,0x00000000,0x00000000 
                                    
#--N*PI/2, -32 <= N <= 32, IN A LEADING TERM IN EXT. AND TRAILING
#--TERM IN SGL. NOTE THAT PI IS 64-BIT LONG, THUS N*PI/2 IS AT
#--MOST 69 BITS LONG.
         global    pitbl            
pitbl:                              
         long      0xc0040000,0xc90fdaa2,0x2168c235,0x21800000 
         long      0xc0040000,0xc2c75bcd,0x105d7c23,0xa0d00000 
         long      0xc0040000,0xbc7edcf7,0xff523611,0xa1e80000 
         long      0xc0040000,0xb6365e22,0xee46f000,0x21480000 
         long      0xc0040000,0xafeddf4d,0xdd3ba9ee,0xa1200000 
         long      0xc0040000,0xa9a56078,0xcc3063dd,0x21fc0000 
         long      0xc0040000,0xa35ce1a3,0xbb251dcb,0x21100000 
         long      0xc0040000,0x9d1462ce,0xaa19d7b9,0xa1580000 
         long      0xc0040000,0x96cbe3f9,0x990e91a8,0x21e00000 
         long      0xc0040000,0x90836524,0x88034b96,0x20b00000 
         long      0xc0040000,0x8a3ae64f,0x76f80584,0xa1880000 
         long      0xc0040000,0x83f2677a,0x65ecbf73,0x21c40000 
         long      0xc0030000,0xfb53d14a,0xa9c2f2c2,0x20000000 
         long      0xc0030000,0xeec2d3a0,0x87ac669f,0x21380000 
         long      0xc0030000,0xe231d5f6,0x6595da7b,0xa1300000 
         long      0xc0030000,0xd5a0d84c,0x437f4e58,0x9fc00000 
         long      0xc0030000,0xc90fdaa2,0x2168c235,0x21000000 
         long      0xc0030000,0xbc7edcf7,0xff523611,0xa1680000 
         long      0xc0030000,0xafeddf4d,0xdd3ba9ee,0xa0a00000 
         long      0xc0030000,0xa35ce1a3,0xbb251dcb,0x20900000 
         long      0xc0030000,0x96cbe3f9,0x990e91a8,0x21600000 
         long      0xc0030000,0x8a3ae64f,0x76f80584,0xa1080000 
         long      0xc0020000,0xfb53d14a,0xa9c2f2c2,0x1f800000 
         long      0xc0020000,0xe231d5f6,0x6595da7b,0xa0b00000 
         long      0xc0020000,0xc90fdaa2,0x2168c235,0x20800000 
         long      0xc0020000,0xafeddf4d,0xdd3ba9ee,0xa0200000 
         long      0xc0020000,0x96cbe3f9,0x990e91a8,0x20e00000 
         long      0xc0010000,0xfb53d14a,0xa9c2f2c2,0x1f000000 
         long      0xc0010000,0xc90fdaa2,0x2168c235,0x20000000 
         long      0xc0010000,0x96cbe3f9,0x990e91a8,0x20600000 
         long      0xc0000000,0xc90fdaa2,0x2168c235,0x1f800000 
         long      0xbfff0000,0xc90fdaa2,0x2168c235,0x1f000000 
         long      0x00000000,0x00000000,0x00000000,0x00000000 
         long      0x3fff0000,0xc90fdaa2,0x2168c235,0x9f000000 
         long      0x40000000,0xc90fdaa2,0x2168c235,0x9f800000 
         long      0x40010000,0x96cbe3f9,0x990e91a8,0xa0600000 
         long      0x40010000,0xc90fdaa2,0x2168c235,0xa0000000 
         long      0x40010000,0xfb53d14a,0xa9c2f2c2,0x9f000000 
         long      0x40020000,0x96cbe3f9,0x990e91a8,0xa0e00000 
         long      0x40020000,0xafeddf4d,0xdd3ba9ee,0x20200000 
         long      0x40020000,0xc90fdaa2,0x2168c235,0xa0800000 
         long      0x40020000,0xe231d5f6,0x6595da7b,0x20b00000 
         long      0x40020000,0xfb53d14a,0xa9c2f2c2,0x9f800000 
         long      0x40030000,0x8a3ae64f,0x76f80584,0x21080000 
         long      0x40030000,0x96cbe3f9,0x990e91a8,0xa1600000 
         long      0x40030000,0xa35ce1a3,0xbb251dcb,0xa0900000 
         long      0x40030000,0xafeddf4d,0xdd3ba9ee,0x20a00000 
         long      0x40030000,0xbc7edcf7,0xff523611,0x21680000 
         long      0x40030000,0xc90fdaa2,0x2168c235,0xa1000000 
         long      0x40030000,0xd5a0d84c,0x437f4e58,0x1fc00000 
         long      0x40030000,0xe231d5f6,0x6595da7b,0x21300000 
         long      0x40030000,0xeec2d3a0,0x87ac669f,0xa1380000 
         long      0x40030000,0xfb53d14a,0xa9c2f2c2,0xa0000000 
         long      0x40040000,0x83f2677a,0x65ecbf73,0xa1c40000 
         long      0x40040000,0x8a3ae64f,0x76f80584,0x21880000 
         long      0x40040000,0x90836524,0x88034b96,0xa0b00000 
         long      0x40040000,0x96cbe3f9,0x990e91a8,0xa1e00000 
         long      0x40040000,0x9d1462ce,0xaa19d7b9,0x21580000 
         long      0x40040000,0xa35ce1a3,0xbb251dcb,0xa1100000 
         long      0x40040000,0xa9a56078,0xcc3063dd,0xa1fc0000 
         long      0x40040000,0xafeddf4d,0xdd3ba9ee,0x21200000 
         long      0x40040000,0xb6365e22,0xee46f000,0xa1480000 
         long      0x40040000,0xbc7edcf7,0xff523611,0x21e80000 
         long      0x40040000,0xc2c75bcd,0x105d7c23,0x20d00000 
         long      0x40040000,0xc90fdaa2,0x2168c235,0xa1800000 
                                    
         set       inarg,fp_scr4    
                                    
         set       twoto63,l_scr1   
         set       endflag,l_scr2   
         set       n,l_scr3         
                                    
                                    
                                    
                                    
         global    stand            
stand:                              
#--TAN(X) = X FOR DENORMALIZED X
                                    
         bra.l     t_extdnrm        
                                    
         global    stan             
stan:                               
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     tanok1           
         bra.w     tansm            
tanok1:                             
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     tanmain          
         bra.w     reducex          
                                    
                                    
tanmain:                            
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
         fmul.d    twobypi,%fp1     # X*2/PI
                                    
#--HIDE THE NEXT TWO INSTRUCTIONS
         lea.l     pitbl+0x200,%a1  # ,32
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,%d0         # CONVERT TO INTEGER
                                    
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # ADDRESS N*PIBY2 IN Y1, Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
#--HIDE THE NEXT ONE
                                    
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
         ror.l     &5,%d0           
         andi.l    &0x80000000,%d0  # D0 WAS ODD IFF D0 < 0
                                    
tancont:                            
                                    
         cmpi.l    %d0,&0           
         blt.w     nodd             
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # S = R*R
                                    
         fmove.d   tanq4,%fp3       
         fmove.d   tanp3,%fp2       
                                    
         fmul.x    %fp1,%fp3        # SQ4
         fmul.x    %fp1,%fp2        # SP3
                                    
         fadd.d    tanq3,%fp3       # Q3+SQ4
         fadd.x    tanp2,%fp2       # P2+SP3
                                    
         fmul.x    %fp1,%fp3        # S(Q3+SQ4)
         fmul.x    %fp1,%fp2        # S(P2+SP3)
                                    
         fadd.x    tanq2,%fp3       # Q2+S(Q3+SQ4)
         fadd.x    tanp1,%fp2       # P1+S(P2+SP3)
                                    
         fmul.x    %fp1,%fp3        # S(Q2+S(Q3+SQ4))
         fmul.x    %fp1,%fp2        # S(P1+S(P2+SP3))
                                    
         fadd.x    tanq1,%fp3       # Q1+S(Q2+S(Q3+SQ4))
         fmul.x    %fp0,%fp2        # RS(P1+S(P2+SP3))
                                    
         fmul.x    %fp3,%fp1        # S(Q1+S(Q2+S(Q3+SQ4)))
                                    
                                    
         fadd.x    %fp2,%fp0        # R+RS(P1+S(P2+SP3))
                                    
                                    
         fadd.s    &0f1.0,%fp1      # )
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fdiv.x    %fp1,%fp0        # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
nodd:                               
         fmove.x   %fp0,%fp1        
         fmul.x    %fp0,%fp0        # S = R*R
                                    
         fmove.d   tanq4,%fp3       
         fmove.d   tanp3,%fp2       
                                    
         fmul.x    %fp0,%fp3        # SQ4
         fmul.x    %fp0,%fp2        # SP3
                                    
         fadd.d    tanq3,%fp3       # Q3+SQ4
         fadd.x    tanp2,%fp2       # P2+SP3
                                    
         fmul.x    %fp0,%fp3        # S(Q3+SQ4)
         fmul.x    %fp0,%fp2        # S(P2+SP3)
                                    
         fadd.x    tanq2,%fp3       # Q2+S(Q3+SQ4)
         fadd.x    tanp1,%fp2       # P1+S(P2+SP3)
                                    
         fmul.x    %fp0,%fp3        # S(Q2+S(Q3+SQ4))
         fmul.x    %fp0,%fp2        # S(P1+S(P2+SP3))
                                    
         fadd.x    tanq1,%fp3       # Q1+S(Q2+S(Q3+SQ4))
         fmul.x    %fp1,%fp2        # RS(P1+S(P2+SP3))
                                    
         fmul.x    %fp3,%fp0        # S(Q1+S(Q2+S(Q3+SQ4)))
                                    
                                    
         fadd.x    %fp2,%fp1        # R+RS(P1+S(P2+SP3))
         fadd.s    &0f1.0,%fp0      # )
                                    
                                    
         fmove.x   %fp1,-(%sp)      
         eori.l    &0x80000000,(%sp) 
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fdiv.x    (%sp)+,%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
tanbors:                            
#--IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION.
#--IF |X| < 2**(-40), RETURN X OR 1.
         cmpi.l    %d0,&0x3fff8000  
         bgt.b     reducex          
                                    
tansm:                              
                                    
         fmove.x   %fp0,-(%sp)      
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   (%sp)+,%fp0      # last inst - posibble exception set
                                    
         bra.l     t_frcinx         
                                    
                                    
reducex:                            
#--WHEN REDUCEX IS USED, THE CODE WILL INEVITABLY BE SLOW.
#--THIS REDUCTION METHOD, HOWEVER, IS MUCH FASTER THAN USING
#--THE REMAINDER INSTRUCTION WHICH IS NOW IN SOFTWARE.
                                    
         fmovem.x  %fp2-%fp5,-(%a7) # save FP2 through FP5
         move.l    %d2,-(%a7)       
         fmove.s   &0f0.0,%fp1      
                                    
#--If compact form of abs(arg) in d0=$7ffeffff, argument is so large that
#--there is a danger of unwanted overflow in first LOOP iteration.  In this
#--case, reduce argument by one remainder step to make subsequent reduction
#--safe.
         cmpi.l    %d0,&0x7ffeffff  # is argument dangerously large?
         bne.b     loop             
         move.l    &0x7ffe0000,fp_scr2(%a6) # yes
#					;create 2**16383*PI/2
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   
         ftest.x   %fp0             # test sign of argument
         move.l    &0x7fdc0000,fp_scr3(%a6) # create low half of 2**16383*
#					;PI/2 at FP_SCR3
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   
         fblt.w    red_neg          
         or.w      &0x8000,fp_scr2(%a6) # positive arg
         or.w      &0x8000,fp_scr3(%a6) 
red_neg:                            
         fadd.x    fp_scr2(%a6),%fp0 # high part of reduction is exact
         fmove.x   %fp0,%fp1        # save high result in fp1
         fadd.x    fp_scr3(%a6),%fp0 # low part of reduction
         fsub.x    %fp0,%fp1        # determine low component of result
         fadd.x    fp_scr3(%a6),%fp1 # fp0/fp1 are reduced argument.
                                    
#--ON ENTRY, FP0 IS X, ON RETURN, FP0 IS X REM PI/2, |X| <= PI/4.
#--integer quotient will be stored in N
#--Intermeditate remainder is 66-bit long; (R,r) in (FP0,FP1)
                                    
loop:                               
         fmove.x   %fp0,inarg(%a6)  # +-2**K * F, 1 <= F < 2
         move.w    inarg(%a6),%d0   
         move.l    %d0,%a1          # save a copy of D0
         andi.l    &0x00007fff,%d0  
         subi.l    &0x00003fff,%d0  # D0 IS K
         cmpi.l    %d0,&28          
         ble.b     lastloop         
contloop:                            
         subi.l    &27,%d0          # D0 IS L := K-27
         move.l    &0,endflag(%a6)  
         bra.b     work             
lastloop:                            
         clr.l     %d0              # D0 IS L := 0
         move.l    &1,endflag(%a6)  
                                    
work:                               
#--FIND THE REMAINDER OF (R,r) W.R.T.	2**L * (PI/2). L IS SO CHOSEN
#--THAT	INT( X * (2/PI) / 2**(L) ) < 2**29.
                                    
#--CREATE 2**(-L) * (2/PI), SIGN(INARG)*2**(63),
#--2**L * (PIby2_1), 2**L * (PIby2_2)
                                    
         move.l    &0x00003ffe,%d2  # BIASED EXPO OF 2/PI
         sub.l     %d0,%d2          # BIASED EXPO OF 2**(-L)*(2/PI)
                                    
         move.l    &0xa2f9836e,fp_scr1+4(%a6) 
         move.l    &0x4e44152a,fp_scr1+8(%a6) 
         move.w    %d2,fp_scr1(%a6) # FP_SCR1 is 2**(-L)*(2/PI)
                                    
         fmove.x   %fp0,%fp2        
         fmul.x    fp_scr1(%a6),%fp2 
#--WE MUST NOW FIND INT(FP2). SINCE WE NEED THIS VALUE IN
#--FLOATING POINT FORMAT, THE TWO FMOVE'S	FMOVE.L FP <--> N
#--WILL BE TOO INEFFICIENT. THE WAY AROUND IT IS THAT
#--(SIGN(INARG)*2**63	+	FP2) - SIGN(INARG)*2**63 WILL GIVE
#--US THE DESIRED VALUE IN FLOATING POINT.
                                    
#--HIDE SIX CYCLES OF INSTRUCTION
         move.l    %a1,%d2          
         swap      %d2              
         andi.l    &0x80000000,%d2  
         ori.l     &0x5f000000,%d2  # D2 IS SIGN(INARG)*2**63 IN SGL
         move.l    %d2,twoto63(%a6) 
                                    
         move.l    %d0,%d2          
         addi.l    &0x00003fff,%d2  # BIASED EXPO OF 2**L * (PI/2)
                                    
#--FP2 IS READY
         fadd.s    twoto63(%a6),%fp2 # THE FRACTIONAL PART OF FP1 IS ROUNDED
                                    
#--HIDE 4 CYCLES OF INSTRUCTION; creating 2**(L)*Piby2_1  and  2**(L)*Piby2_2
         move.w    %d2,fp_scr2(%a6) 
         clr.w     fp_scr2+2(%a6)   
         move.l    &0xc90fdaa2,fp_scr2+4(%a6) 
         clr.l     fp_scr2+8(%a6)   # FP_SCR2 is  2**(L) * Piby2_1	
                                    
#--FP2 IS READY
         fsub.s    twoto63(%a6),%fp2 # FP2 is N
                                    
         addi.l    &0x00003fdd,%d0  
         move.w    %d0,fp_scr3(%a6) 
         clr.w     fp_scr3+2(%a6)   
         move.l    &0x85a308d3,fp_scr3+4(%a6) 
         clr.l     fp_scr3+8(%a6)   # FP_SCR3 is 2**(L) * Piby2_2
                                    
         move.l    endflag(%a6),%d0 
                                    
#--We are now ready to perform (R+r) - N*P1 - N*P2, P1 = 2**(L) * Piby2_1 and
#--P2 = 2**(L) * Piby2_2
         fmove.x   %fp2,%fp4        
         fmul.x    fp_scr2(%a6),%fp4 # W = N*P1
         fmove.x   %fp2,%fp5        
         fmul.x    fp_scr3(%a6),%fp5 # w = N*P2
         fmove.x   %fp4,%fp3        
#--we want P+p = W+w  but  |p| <= half ulp of P
#--Then, we need to compute  A := R-P   and  a := r-p
         fadd.x    %fp5,%fp3        # FP3 is P
         fsub.x    %fp3,%fp4        # W-P
                                    
         fsub.x    %fp3,%fp0        # FP0 is A := R - P
         fadd.x    %fp5,%fp4        # FP4 is p = (W-P)+w
                                    
         fmove.x   %fp0,%fp3        # FP3 A
         fsub.x    %fp4,%fp1        # FP1 is a := r - p
                                    
#--Now we need to normalize (A,a) to  "new (R,r)" where R+r = A+a but
#--|r| <= half ulp of R.
         fadd.x    %fp1,%fp0        # FP0 is R := A+a
#--No need to calculate r if this is the last loop
         cmpi.l    %d0,&0           
         bgt.w     restore          
                                    
#--Need to calculate r
         fsub.x    %fp0,%fp3        # A-R
         fadd.x    %fp3,%fp1        # FP1 is r := (A-R)+a
         bra.w     loop             
                                    
restore:                            
         fmove.l   %fp2,n(%a6)      
         move.l    (%a7)+,%d2       
         fmovem.x  (%a7)+,%fp2-%fp5 
                                    
                                    
         move.l    n(%a6),%d0       
         ror.l     &1,%d0           
                                    
                                    
         bra.w     tancont          
                                    
                                    
	version 3
