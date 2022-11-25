ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/ssin.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:53:21 $
                                    
#
#	ssin.sa 3.3 7/29/91
#
#	The entry point sSIN computes the sine of an input argument
#	sCOS computes the cosine, and sSINCOS computes both. The
#	corresponding entry points with a "d" computes the same
#	corresponding function values for denormalized inputs.
#
#	Input: Double-extended number X in location pointed to
#		by address register a0.
#
#	Output: The funtion value sin(X) or cos(X) returned in Fp0 if SIN or
#		COS is requested. Otherwise, for SINCOS, sin(X) is returned
#		in Fp0, and cos(X) is returned in Fp1.
#
#	Modifies: Fp0 for SIN or COS; both Fp0 and Fp1 for SINCOS.
#
#	Accuracy and Monotonicity: The returned result is within 1 ulp in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The programs sSIN and sCOS take approximately 150 cycles for
#		input argument X such that |X| < 15Pi, which is the the usual
#		situation. The speed for sSINCOS is approximately 190 cycles.
#
#	Algorithm:
#
#	SIN and COS:
#	1. If SIN is invoked, set AdjN := 0; otherwise, set AdjN := 1.
#
#	2. If |X| >= 15Pi or |X| < 2**(-40), go to 7.
#
#	3. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 4, so in particular, k = 0,1,2,or 3. Overwirte
#		k by k := k + AdjN.
#
#	4. If k is even, go to 6.
#
#	5. (k is odd) Set j := (k-1)/2, sgn := (-1)**j. Return sgn*cos(r)
#		where cos(r) is approximated by an even polynomial in r,
#		1 + r*r*(B1+s*(B2+ ... + s*B8)),	s = r*r.
#		Exit.
#
#	6. (k is even) Set j := k/2, sgn := (-1)**j. Return sgn*sin(r)
#		where sin(r) is approximated by an odd polynomial in r
#		r + r*s*(A1+s*(A2+ ... + s*A7)),	s = r*r.
#		Exit.
#
#	7. If |X| > 1, go to 9.
#
#	8. (|X|<2**(-40)) If SIN is invoked, return X; otherwise return 1.
#
#	9. Overwrite X by X := X rem 2Pi. Now that |X| <= Pi, go back to 3.
#
#	SINCOS:
#	1. If |X| >= 15Pi or |X| < 2**(-40), go to 6.
#
#	2. Decompose X as X = N(Pi/2) + r where |r| <= Pi/4. Let
#		k = N mod 4, so in particular, k = 0,1,2,or 3.
#
#	3. If k is even, go to 5.
#
#	4. (k is odd) Set j1 := (k-1)/2, j2 := j1 (EOR) (k mod 2), i.e.
#		j1 exclusive or with the l.s.b. of k.
#		sgn1 := (-1)**j1, sgn2 := (-1)**j2.
#		SIN(X) = sgn1 * cos(r) and COS(X) = sgn2*sin(r) where
#		sin(r) and cos(r) are computed as odd and even polynomials
#		in r, respectively. Exit
#
#	5. (k is even) Set j1 := k/2, sgn1 := (-1)**j1.
#		SIN(X) = sgn1 * sin(r) and COS(X) = sgn1*cos(r) where
#		sin(r) and cos(r) are computed as odd and even polynomials
#		in r, respectively. Exit
#
#	6. If |X| > 1, go to 8.
#
#	7. (|X|<2**(-40)) SIN(X) = X and COS(X) = 1. Exit.
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
                                    
sina7:   long      0xbd6aaa77,0xccc994f5 
sina6:   long      0x3de61209,0x7aae8da1 
                                    
sina5:   long      0xbe5ae645,0x2a118ae4 
sina4:   long      0x3ec71de3,0xa5341531 
                                    
sina3:   long      0xbf2a01a0,0x1a018b59,0x00000000,0x00000000 
                                    
sina2:   long      0x3ff80000,0x88888888,0x888859af,0x00000000 
                                    
sina1:   long      0xbffc0000,0xaaaaaaaa,0xaaaaaa99,0x00000000 
                                    
cosb8:   long      0x3d2ac4d0,0xd6011ee3 
cosb7:   long      0xbda9396f,0x9f45ac19 
                                    
cosb6:   long      0x3e21eed9,0x0612c972 
cosb5:   long      0xbe927e4f,0xb79d9fcf 
                                    
cosb4:   long      0x3efa01a0,0x1a01d423,0x00000000,0x00000000 
                                    
cosb3:   long      0xbff50000,0xb60b60b6,0x0b61d438,0x00000000 
                                    
cosb2:   long      0x3ffa0000,0xaaaaaaaa,0xaaaaab5e 
cosb1:   long      0xbf000000       
                                    
invtwopi: long      0x3ffc0000,0xa2f9836e,0x4e44152a 
                                    
twopi1:  long      0x40010000,0xc90fdaa2,0x00000000,0x00000000 
twopi2:  long      0x3fdf0000,0x85a308d4,0x00000000,0x00000000 
                                    
                                    
                                    
         set       inarg,fp_scr4    
                                    
         set       x,fp_scr5        
         set       xdcare,x+2       
         set       xfrac,x+4        
                                    
         set       rprime,fp_scr1   
         set       sprime,fp_scr2   
                                    
         set       posneg1,l_scr1   
         set       twoto63,l_scr1   
                                    
         set       endflag,l_scr2   
         set       n,l_scr2         
                                    
         set       adjn,l_scr3      
                                    
                                    
                                    
                                    
                                    
         global    ssind            
ssind:                              
#--SIN(X) = X FOR DENORMALIZED X
         bra.l     t_extdnrm        
                                    
         global    scosd            
scosd:                              
#--COS(X) = 1 FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp0      
#
#	9D25B Fix: Sometimes the previous fmove.s sets fpsr bits
#
         fmove.l   &0,%fpsr         
#
         bra.l     t_frcinx         
                                    
         global    ssin             
ssin:                               
#--SET ADJN TO 0
         move.l    &0,adjn(%a6)     
         bra.b     sinbgn           
                                    
         global    scos             
scos:                               
#--SET ADJN TO 1
         move.l    &1,adjn(%a6)     
                                    
sinbgn:                             
#--SAVE FPCR, FP1. CHECK IF |X| IS TOO SMALL OR LARGE
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,x(%a6)      
         andi.l    &0x7fffffff,%d0  # COMPACTIFY X
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     sok1             
         bra.w     sinsm            
                                    
sok1:                               
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     sinmain          
         bra.w     reducex          
                                    
sinmain:                            
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
         fmul.d    twobypi,%fp1     # X*2/PI
                                    
#--HIDE THE NEXT THREE INSTRUCTIONS
         lea       pitbl+0x200,%a1  # ,32
                                    
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,n(%a6)      # CONVERT TO INTEGER
                                    
         move.l    n(%a6),%d0       
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # A1 IS THE ADDRESS OF N*PIBY2
#				...WHICH IS IN TWO PIECES Y1 & Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
#--HIDE THE NEXT ONE
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
sincont:                            
#--continuation from REDUCEX
                                    
#--GET N+ADJN AND SEE IF SIN(R) OR COS(R) IS NEEDED
         move.l    n(%a6),%d0       
         add.l     adjn(%a6),%d0    # SEE IF D0 IS ODD OR EVEN
         ror.l     &1,%d0           # D0 WAS ODD IFF D0 IS NEGATIVE
         cmpi.l    %d0,&0           
         blt.w     cospoly          
                                    
sinpoly:                            
#--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
#--THEN WE RETURN	SGN*SIN(R). SGN*SIN(R) IS COMPUTED BY
#--R' + R'*S*(A1 + S(A2 + S(A3 + S(A4 + ... + SA7)))), WHERE
#--R' = SGN*R, S=R*R. THIS CAN BE REWRITTEN AS
#--R' + R'*S*( [A1+T(A3+T(A5+TA7))] + [S(A2+T(A4+TA6))])
#--WHERE T=S*S.
#--NOTE THAT A3 THROUGH A7 ARE STORED IN DOUBLE PRECISION
#--WHILE A1 AND A2 ARE IN DOUBLE-EXTENDED FORMAT.
         fmove.x   %fp0,x(%a6)      # X IS R
         fmul.x    %fp0,%fp0        # FP0 IS S
#---HIDE THE NEXT TWO WHILE WAITING FOR FP0
         fmove.d   sina7,%fp3       
         fmove.d   sina6,%fp2       
#--FP0 IS NOW READY
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS T
#--HIDE THE NEXT TWO WHILE WAITING FOR FP1
                                    
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
#				...LEAST SIG. BIT OF D0 IN SIGN POSITION
         eor.l     %d0,x(%a6)       # X IS NOW R'= SGN*R
                                    
         fmul.x    %fp1,%fp3        # TA7
         fmul.x    %fp1,%fp2        # TA6
                                    
         fadd.d    sina5,%fp3       # A5+TA7
         fadd.d    sina4,%fp2       # A4+TA6
                                    
         fmul.x    %fp1,%fp3        # T(A5+TA7)
         fmul.x    %fp1,%fp2        # T(A4+TA6)
                                    
         fadd.d    sina3,%fp3       # A3+T(A5+TA7)
         fadd.x    sina2,%fp2       # A2+T(A4+TA6)
                                    
         fmul.x    %fp3,%fp1        # T(A3+T(A5+TA7))
                                    
         fmul.x    %fp0,%fp2        # S(A2+T(A4+TA6))
         fadd.x    sina1,%fp1       # A1+T(A3+T(A5+TA7))
         fmul.x    x(%a6),%fp0      # R'*S
                                    
         fadd.x    %fp2,%fp1        # [A1+T(A3+T(A5+TA7))]+[S(A2+T(A4+TA6))]
#--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
#--FP2 RELEASED, RESTORE NOW AND TAKE FULL ADVANTAGE OF HIDING
                                    
                                    
         fmul.x    %fp1,%fp0        # SIN(R__hpasm_string1
#--FP1 RELEASED.
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    x(%a6),%fp0      # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
cospoly:                            
#--LET J BE THE LEAST SIG. BIT OF D0, LET SGN := (-1)**J.
#--THEN WE RETURN	SGN*COS(R). SGN*COS(R) IS COMPUTED BY
#--SGN + S'*(B1 + S(B2 + S(B3 + S(B4 + ... + SB8)))), WHERE
#--S=R*R AND S'=SGN*S. THIS CAN BE REWRITTEN AS
#--SGN + S'*([B1+T(B3+T(B5+TB7))] + [S(B2+T(B4+T(B6+TB8)))])
#--WHERE T=S*S.
#--NOTE THAT B4 THROUGH B8 ARE STORED IN DOUBLE PRECISION
#--WHILE B2 AND B3 ARE IN DOUBLE-EXTENDED FORMAT, B1 IS -1/2
#--AND IS THEREFORE STORED AS SINGLE PRECISION.
                                    
         fmul.x    %fp0,%fp0        # FP0 IS S
#---HIDE THE NEXT TWO WHILE WAITING FOR FP0
         fmove.d   cosb8,%fp2       
         fmove.d   cosb7,%fp3       
#--FP0 IS NOW READY
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS T
#--HIDE THE NEXT TWO WHILE WAITING FOR FP1
         fmove.x   %fp0,x(%a6)      # X IS S
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
#			...LEAST SIG. BIT OF D0 IN SIGN POSITION
                                    
         fmul.x    %fp1,%fp2        # TB8
#--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
         eor.l     %d0,x(%a6)       # X IS NOW S'= SGN*S
         andi.l    &0x80000000,%d0  
                                    
         fmul.x    %fp1,%fp3        # TB7
#--HIDE THE NEXT TWO WHILE WAITING FOR THE XU
         ori.l     &0x3f800000,%d0  # D0 IS SGN IN SINGLE
         move.l    %d0,posneg1(%a6) 
                                    
         fadd.d    cosb6,%fp2       # B6+TB8
         fadd.d    cosb5,%fp3       # B5+TB7
                                    
         fmul.x    %fp1,%fp2        # T(B6+TB8)
         fmul.x    %fp1,%fp3        # T(B5+TB7)
                                    
         fadd.d    cosb4,%fp2       # B4+T(B6+TB8)
         fadd.x    cosb3,%fp3       # B3+T(B5+TB7)
                                    
         fmul.x    %fp1,%fp2        # T(B4+T(B6+TB8))
         fmul.x    %fp3,%fp1        # T(B3+T(B5+TB7))
                                    
         fadd.x    cosb2,%fp2       # B2+T(B4+T(B6+TB8))
         fadd.s    cosb1,%fp1       # B1+T(B3+T(B5+TB7))
                                    
         fmul.x    %fp2,%fp0        # S(B2+T(B4+T(B6+TB8)))
#--FP3 RELEASED, RESTORE NOW AND TAKE SOME ADVANTAGE OF HIDING
#--FP2 RELEASED.
                                    
                                    
         fadd.x    %fp1,%fp0        
#--FP1 RELEASED
                                    
         fmul.x    x(%a6),%fp0      
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.s    posneg1(%a6),%fp0 # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
sinbors:                            
#--IF |X| > 15PI, WE USE THE GENERAL ARGUMENT REDUCTION.
#--IF |X| < 2**(-40), RETURN X OR 1.
         cmpi.l    %d0,&0x3fff8000  
         bgt.b     reducex          
                                    
                                    
sinsm:                              
         move.l    adjn(%a6),%d0    
         cmpi.l    %d0,&0           
         bgt.b     costiny          
                                    
sintiny:                            
         move.w    &0x0000,xdcare(%a6) # JUST IN CASE
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   x(%a6),%fp0      # last inst - possible exception set
         bra.l     t_frcinx         
                                    
                                    
costiny:                            
         fmove.s   &0f1.0,%fp0      
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fsub.s    &0f1.1754943e-38,%fp0 # last inst - possible exception set
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
                                    
                                    
         move.l    adjn(%a6),%d0    
         cmpi.l    %d0,&4           
                                    
         blt.w     sincont          
         bra.b     sccont           
                                    
         global    ssincosd         
ssincosd:                            
#--SIN AND COS OF X FOR DENORMALIZED X
                                    
         fmove.s   &0f1.0,%fp1      
         bsr.l     sto_cos          # store cosine result
         bra.l     t_extdnrm        
                                    
         global    ssincos          
ssincos:                            
#--SET ADJN TO 4
         move.l    &4,adjn(%a6)     
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,x(%a6)      
         andi.l    &0x7fffffff,%d0  # COMPACTIFY X
                                    
         cmpi.l    %d0,&0x3fd78000  # |X| >= 2**(-40)?
         bge.b     scok1            
         bra.w     scsm             
                                    
scok1:                              
         cmpi.l    %d0,&0x4004bc7e  # |X| < 15 PI?
         blt.b     scmain           
         bra.w     reducex          
                                    
                                    
scmain:                             
#--THIS IS THE USUAL CASE, |X| <= 15 PI.
#--THE ARGUMENT REDUCTION IS DONE BY TABLE LOOK UP.
         fmove.x   %fp0,%fp1        
         fmul.d    twobypi,%fp1     # X*2/PI
                                    
#--HIDE THE NEXT THREE INSTRUCTIONS
         lea       pitbl+0x200,%a1  # ,32
                                    
                                    
#--FP1 IS NOW READY
         fmove.l   %fp1,n(%a6)      # CONVERT TO INTEGER
                                    
         move.l    n(%a6),%d0       
         asl.l     &4,%d0           
         adda.l    %d0,%a1          # ADDRESS OF N*PIBY2, IN Y1, Y2
                                    
         fsub.x    (%a1)+,%fp0      # X-Y1
         fsub.s    (%a1),%fp0       # FP0 IS R = (X-Y1)-Y2
                                    
sccont:                             
#--continuation point from REDUCEX
                                    
#--HIDE THE NEXT TWO
         move.l    n(%a6),%d0       
         ror.l     &1,%d0           
                                    
         cmpi.l    %d0,&0           # D0 < 0 IFF N IS ODD
         bge.w     neven            
                                    
nodd:                               
#--REGISTERS SAVED SO FAR: D0, A0, FP2.
                                    
         fmove.x   %fp0,rprime(%a6) 
         fmul.x    %fp0,%fp0        # FP0 IS S = R*R
         fmove.d   sina7,%fp1       # A7
         fmove.d   cosb8,%fp2       # B8
         fmul.x    %fp0,%fp1        # SA7
         move.l    %d2,-(%a7)       
         move.l    %d0,%d2          
         fmul.x    %fp0,%fp2        # SB8
         ror.l     &1,%d2           
         andi.l    &0x80000000,%d2  
                                    
         fadd.d    sina6,%fp1       # A6+SA7
         eor.l     %d0,%d2          
         andi.l    &0x80000000,%d2  
         fadd.d    cosb7,%fp2       # B7+SB8
                                    
         fmul.x    %fp0,%fp1        # S(A6+SA7)
         eor.l     %d2,rprime(%a6)  
         move.l    (%a7)+,%d2       
         fmul.x    %fp0,%fp2        # S(B7+SB8)
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
                                    
         fadd.d    sina5,%fp1       # A5+S(A6+SA7)
         move.l    &0x3f800000,posneg1(%a6) 
         eor.l     %d0,posneg1(%a6) 
         fadd.d    cosb6,%fp2       # B6+S(B7+SB8)
                                    
         fmul.x    %fp0,%fp1        # S(A5+S(A6+SA7))
         fmul.x    %fp0,%fp2        # S(B6+S(B7+SB8))
         fmove.x   %fp0,sprime(%a6) 
                                    
         fadd.d    sina4,%fp1       # A4+S(A5+S(A6+SA7))
         eor.l     %d0,sprime(%a6)  
         fadd.d    cosb5,%fp2       # B5+S(B6+S(B7+SB8))
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.d    sina3,%fp1       # )
         fadd.d    cosb4,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.x    sina2,%fp1       # )
         fadd.x    cosb3,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.x    sina1,%fp1       # )
         fadd.x    cosb2,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp2,%fp0        # )
                                    
                                    
                                    
         fmul.x    rprime(%a6),%fp1 # )
         fadd.s    cosb1,%fp0       # )
         fmul.x    sprime(%a6),%fp0 # ))
                                    
         move.l    %d1,-(%sp)       # restore users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fadd.x    rprime(%a6),%fp1 # COS(X)
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # restore users exceptions
         fadd.s    posneg1(%a6),%fp0 # SIN(X)
                                    
         bra.l     t_frcinx         
                                    
                                    
neven:                              
#--REGISTERS SAVED SO FAR: FP2.
                                    
         fmove.x   %fp0,rprime(%a6) 
         fmul.x    %fp0,%fp0        # FP0 IS S = R*R
         fmove.d   cosb8,%fp1       # B8
         fmove.d   sina7,%fp2       # A7
         fmul.x    %fp0,%fp1        # SB8
         fmove.x   %fp0,sprime(%a6) 
         fmul.x    %fp0,%fp2        # SA7
         ror.l     &1,%d0           
         andi.l    &0x80000000,%d0  
         fadd.d    cosb7,%fp1       # B7+SB8
         fadd.d    sina6,%fp2       # A6+SA7
         eor.l     %d0,rprime(%a6)  
         eor.l     %d0,sprime(%a6)  
         fmul.x    %fp0,%fp1        # S(B7+SB8)
         ori.l     &0x3f800000,%d0  
         move.l    %d0,posneg1(%a6) 
         fmul.x    %fp0,%fp2        # S(A6+SA7)
                                    
         fadd.d    cosb6,%fp1       # B6+S(B7+SB8)
         fadd.d    sina5,%fp2       # A5+S(A6+SA7)
                                    
         fmul.x    %fp0,%fp1        # S(B6+S(B7+SB8))
         fmul.x    %fp0,%fp2        # S(A5+S(A6+SA7))
                                    
         fadd.d    cosb5,%fp1       # B5+S(B6+S(B7+SB8))
         fadd.d    sina4,%fp2       # A4+S(A5+S(A6+SA7))
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.d    cosb4,%fp1       # )
         fadd.d    sina3,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.x    cosb3,%fp1       # )
         fadd.x    sina2,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp0,%fp2        # )
                                    
         fadd.x    cosb2,%fp1       # )
         fadd.x    sina1,%fp2       # )
                                    
         fmul.x    %fp0,%fp1        # )
         fmul.x    %fp2,%fp0        # )
                                    
                                    
                                    
         fadd.s    cosb1,%fp1       # )
         fmul.x    rprime(%a6),%fp0 # )
         fmul.x    sprime(%a6),%fp1 # ))
                                    
         move.l    %d1,-(%sp)       # save users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fadd.s    posneg1(%a6),%fp1 # COS(X)
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # restore users exceptions
         fadd.x    rprime(%a6),%fp0 # SIN(X)
                                    
         bra.l     t_frcinx         
                                    
scbors:                             
         cmpi.l    %d0,&0x3fff8000  
         bgt.w     reducex          
                                    
                                    
scsm:                               
         move.w    &0x0000,xdcare(%a6) 
         fmove.s   &0f1.0,%fp1      
                                    
         move.l    %d1,-(%sp)       # save users mode & precision
         andi.l    &0xff,%d1        # mask off all exceptions
         fmove.l   %d1,%fpcr        
         fsub.s    &0f1.1754943e-38,%fp1 
         bsr.l     sto_cos          # store cosine result
         fmove.l   (%sp)+,%fpcr     # restore users exceptions
         fmove.x   x(%a6),%fp0      
         bra.l     t_frcinx         
                                    
                                    
	version 3
