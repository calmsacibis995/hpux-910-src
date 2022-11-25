ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/slogn.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:52:37 $
                                    
#
#	slogn.sa 3.1 12/10/90
#
#	slogn computes the natural logarithm of an
#	input value. slognd does the same except the input value is a
#	denormalized number. slognp1 computes log(1+X), and slognp1d
#	computes log(1+X) for denormalized X.
#
#	Input: Double-extended value in memory location pointed to by address
#		register a0.
#
#	Output:	log(X) or log(1+X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 2 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The 
#		result is provably monotonic in double precision.
#
#	Speed: The program slogn takes approximately 190 cycles for input 
#		argument X such that |X-1| >= 1/16, which is the the usual 
#		situation. For those arguments, slognp1 takes approximately
#		 210 cycles. For the less common arguments, the program will
#		 run no worse than 10% slower.
#
#	Algorithm:
#	LOGN:
#	Step 1. If |X-1| < 1/16, approximate log(X) by an odd polynomial in
#		u, where u = 2(X-1)/(X+1). Otherwise, move on to Step 2.
#
#	Step 2. X = 2**k * Y where 1 <= Y < 2. Define F to be the first seven
#		significant bits of Y plus 2**(-7), i.e. F = 1.xxxxxx1 in base
#		2 where the six "x" match those of Y. Note that |Y-F| <= 2**(-7).
#
#	Step 3. Define u = (Y-F)/F. Approximate log(1+u) by a polynomial in u,
#		log(1+u) = poly.
#
#	Step 4. Reconstruct log(X) = log( 2**k * Y ) = k*log(2) + log(F) + log(1+u)
#		by k*log(2) + (log(F) + poly). The values of log(F) are calculated
#		beforehand and stored in the program.
#
#	lognp1:
#	Step 1: If |X| < 1/16, approximate log(1+X) by an odd polynomial in
#		u where u = 2X/(2+X). Otherwise, move on to Step 2.
#
#	Step 2: Let 1+X = 2**k * Y, where 1 <= Y < 2. Define F as done in Step 2
#		of the algorithm for LOGN and compute log(1+X) as
#		k*log(2) + log(F) + poly where poly approximates log(1+u),
#		u = (Y-F)/F. 
#
#	Implementation Notes:
#	Note 1. There are 64 different possible values for F, thus 64 log(F)'s
#		need to be tabulated. Moreover, the values of 1/F are also 
#		tabulated so that the division in (Y-F)/F can be performed by a
#		multiplication.
#
#	Note 2. In Step 2 of lognp1, in order to preserved accuracy, the value
#		Y-F has to be calculated carefully when 1/2 <= X < 3/2. 
#
#	Note 3. To fully exploit the pipeline, polynomials are usually separated
#		into two parts evaluated independently before being added up.
#	
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
bounds1: long      0x3ffef07d,0x3fff8841 
bounds2: long      0x3ffe8000,0x3fffc000 
                                    
logof2:  long      0x3ffe0000,0xb17217f7,0xd1cf79ac,0x00000000 
                                    
one:     long      0x3f800000       
zero:    long      0x00000000       
infty:   long      0x7f800000       
negone:  long      0xbf800000       
                                    
loga6:   long      0x3fc2499a,0xb5e4040b 
loga5:   long      0xbfc555b5,0x848cb7db 
                                    
loga4:   long      0x3fc99999,0x987d8730 
loga3:   long      0xbfcfffff,0xff6f7e97 
                                    
loga2:   long      0x3fd55555,0x555555a4 
loga1:   long      0xbfe00000,0x00000008 
                                    
logb5:   long      0x3f175496,0xadd7dad6 
logb4:   long      0x3f3c71c2,0xfe80c7e0 
                                    
logb3:   long      0x3f624924,0x928bccff 
logb2:   long      0x3f899999,0x999995ec 
                                    
logb1:   long      0x3fb55555,0x55555555 
two:     long      0x40000000,0x00000000 
                                    
lthold:  long      0x3f990000,0x80000000,0x00000000,0x00000000 
                                    
logtbl:                             
         long      0x3ffe0000,0xfe03f80f,0xe03f80fe,0x00000000 
         long      0x3ff70000,0xff015358,0x833c47e2,0x00000000 
         long      0x3ffe0000,0xfa232cf2,0x52138ac0,0x00000000 
         long      0x3ff90000,0xbdc8d83e,0xad88d549,0x00000000 
         long      0x3ffe0000,0xf6603d98,0x0f6603da,0x00000000 
         long      0x3ffa0000,0x9cf43dcf,0xf5eafd48,0x00000000 
         long      0x3ffe0000,0xf2b9d648,0x0f2b9d65,0x00000000 
         long      0x3ffa0000,0xda16eb88,0xcb8df614,0x00000000 
         long      0x3ffe0000,0xef2eb71f,0xc4345238,0x00000000 
         long      0x3ffb0000,0x8b29b775,0x1bd70743,0x00000000 
         long      0x3ffe0000,0xebbdb2a5,0xc1619c8c,0x00000000 
         long      0x3ffb0000,0xa8d839f8,0x30c1fb49,0x00000000 
         long      0x3ffe0000,0xe865ac7b,0x7603a197,0x00000000 
         long      0x3ffb0000,0xc61a2eb1,0x8cd907ad,0x00000000 
         long      0x3ffe0000,0xe525982a,0xf70c880e,0x00000000 
         long      0x3ffb0000,0xe2f2a47a,0xde3a18af,0x00000000 
         long      0x3ffe0000,0xe1fc780e,0x1fc780e2,0x00000000 
         long      0x3ffb0000,0xff64898e,0xdf55d551,0x00000000 
         long      0x3ffe0000,0xdee95c4c,0xa037ba57,0x00000000 
         long      0x3ffc0000,0x8db956a9,0x7b3d0148,0x00000000 
         long      0x3ffe0000,0xdbeb61ee,0xd19c5958,0x00000000 
         long      0x3ffc0000,0x9b8fe100,0xf47ba1de,0x00000000 
         long      0x3ffe0000,0xd901b203,0x6406c80e,0x00000000 
         long      0x3ffc0000,0xa9372f1d,0x0da1bd17,0x00000000 
         long      0x3ffe0000,0xd62b80d6,0x2b80d62c,0x00000000 
         long      0x3ffc0000,0xb6b07f38,0xce90e46b,0x00000000 
         long      0x3ffe0000,0xd3680d36,0x80d3680d,0x00000000 
         long      0x3ffc0000,0xc3fd0329,0x06488481,0x00000000 
         long      0x3ffe0000,0xd0b69fcb,0xd2580d0b,0x00000000 
         long      0x3ffc0000,0xd11de0ff,0x15ab18ca,0x00000000 
         long      0x3ffe0000,0xce168a77,0x25080ce1,0x00000000 
         long      0x3ffc0000,0xde1433a1,0x6c66b150,0x00000000 
         long      0x3ffe0000,0xcb8727c0,0x65c393e0,0x00000000 
         long      0x3ffc0000,0xeae10b5a,0x7ddc8add,0x00000000 
         long      0x3ffe0000,0xc907da4e,0x871146ad,0x00000000 
         long      0x3ffc0000,0xf7856e5e,0xe2c9b291,0x00000000 
         long      0x3ffe0000,0xc6980c69,0x80c6980c,0x00000000 
         long      0x3ffd0000,0x82012ca5,0xa68206d7,0x00000000 
         long      0x3ffe0000,0xc4372f85,0x5d824ca6,0x00000000 
         long      0x3ffd0000,0x882c5fcd,0x7256a8c5,0x00000000 
         long      0x3ffe0000,0xc1e4bbd5,0x95f6e947,0x00000000 
         long      0x3ffd0000,0x8e44c60b,0x4ccfd7de,0x00000000 
         long      0x3ffe0000,0xbfa02fe8,0x0bfa02ff,0x00000000 
         long      0x3ffd0000,0x944ad09e,0xf4351af6,0x00000000 
         long      0x3ffe0000,0xbd691047,0x07661aa3,0x00000000 
         long      0x3ffd0000,0x9a3eecd4,0xc3eaa6b2,0x00000000 
         long      0x3ffe0000,0xbb3ee721,0xa54d880c,0x00000000 
         long      0x3ffd0000,0xa0218434,0x353f1de8,0x00000000 
         long      0x3ffe0000,0xb92143fa,0x36f5e02e,0x00000000 
         long      0x3ffd0000,0xa5f2fcab,0xbbc506da,0x00000000 
         long      0x3ffe0000,0xb70fbb5a,0x19be3659,0x00000000 
         long      0x3ffd0000,0xabb3b8ba,0x2ad362a5,0x00000000 
         long      0x3ffe0000,0xb509e68a,0x9b94821f,0x00000000 
         long      0x3ffd0000,0xb1641795,0xce3ca97b,0x00000000 
         long      0x3ffe0000,0xb30f6352,0x8917c80b,0x00000000 
         long      0x3ffd0000,0xb7047551,0x5d0f1c61,0x00000000 
         long      0x3ffe0000,0xb11fd3b8,0x0b11fd3c,0x00000000 
         long      0x3ffd0000,0xbc952afe,0xea3d13e1,0x00000000 
         long      0x3ffe0000,0xaf3addc6,0x80af3ade,0x00000000 
         long      0x3ffd0000,0xc2168ed0,0xf458ba4a,0x00000000 
         long      0x3ffe0000,0xad602b58,0x0ad602b6,0x00000000 
         long      0x3ffd0000,0xc788f439,0xb3163bf1,0x00000000 
         long      0x3ffe0000,0xab8f69e2,0x8359cd11,0x00000000 
         long      0x3ffd0000,0xccecac08,0xbf04565d,0x00000000 
         long      0x3ffe0000,0xa9c84a47,0xa07f5638,0x00000000 
         long      0x3ffd0000,0xd2420487,0x2dd85160,0x00000000 
         long      0x3ffe0000,0xa80a80a8,0x0a80a80b,0x00000000 
         long      0x3ffd0000,0xd7894992,0x3bc3588a,0x00000000 
         long      0x3ffe0000,0xa655c439,0x2d7b73a8,0x00000000 
         long      0x3ffd0000,0xdcc2c4b4,0x9887dacc,0x00000000 
         long      0x3ffe0000,0xa4a9cf1d,0x96833751,0x00000000 
         long      0x3ffd0000,0xe1eebd3e,0x6d6a6b9e,0x00000000 
         long      0x3ffe0000,0xa3065e3f,0xae7cd0e0,0x00000000 
         long      0x3ffd0000,0xe70d785c,0x2f9f5bdc,0x00000000 
         long      0x3ffe0000,0xa16b312e,0xa8fc377d,0x00000000 
         long      0x3ffd0000,0xec1f392c,0x5179f283,0x00000000 
         long      0x3ffe0000,0x9fd809fd,0x809fd80a,0x00000000 
         long      0x3ffd0000,0xf12440d3,0xe36130e6,0x00000000 
         long      0x3ffe0000,0x9e4cad23,0xdd5f3a20,0x00000000 
         long      0x3ffd0000,0xf61cce92,0x346600bb,0x00000000 
         long      0x3ffe0000,0x9cc8e160,0xc3fb19b9,0x00000000 
         long      0x3ffd0000,0xfb091fd3,0x8145630a,0x00000000 
         long      0x3ffe0000,0x9b4c6f9e,0xf03a3caa,0x00000000 
         long      0x3ffd0000,0xffe97042,0xbfa4c2ad,0x00000000 
         long      0x3ffe0000,0x99d722da,0xbde58f06,0x00000000 
         long      0x3ffe0000,0x825efced,0x49369330,0x00000000 
         long      0x3ffe0000,0x9868c809,0x868c8098,0x00000000 
         long      0x3ffe0000,0x84c37a7a,0xb9a905c9,0x00000000 
         long      0x3ffe0000,0x97012e02,0x5c04b809,0x00000000 
         long      0x3ffe0000,0x87224c2e,0x8e645fb7,0x00000000 
         long      0x3ffe0000,0x95a02568,0x095a0257,0x00000000 
         long      0x3ffe0000,0x897b8cac,0x9f7de298,0x00000000 
         long      0x3ffe0000,0x94458094,0x45809446,0x00000000 
         long      0x3ffe0000,0x8bcf55de,0xc4cd05fe,0x00000000 
         long      0x3ffe0000,0x92f11384,0x0497889c,0x00000000 
         long      0x3ffe0000,0x8e1dc0fb,0x89e125e5,0x00000000 
         long      0x3ffe0000,0x91a2b3c4,0xd5e6f809,0x00000000 
         long      0x3ffe0000,0x9066e68c,0x955b6c9b,0x00000000 
         long      0x3ffe0000,0x905a3863,0x3e06c43b,0x00000000 
         long      0x3ffe0000,0x92aade74,0xc7be59e0,0x00000000 
         long      0x3ffe0000,0x8f1779d9,0xfdc3a219,0x00000000 
         long      0x3ffe0000,0x94e9bff6,0x15845643,0x00000000 
         long      0x3ffe0000,0x8dda5202,0x37694809,0x00000000 
         long      0x3ffe0000,0x9723a1b7,0x20134203,0x00000000 
         long      0x3ffe0000,0x8ca29c04,0x6514e023,0x00000000 
         long      0x3ffe0000,0x995899c8,0x90eb8990,0x00000000 
         long      0x3ffe0000,0x8b70344a,0x139bc75a,0x00000000 
         long      0x3ffe0000,0x9b88bdaa,0x3a3dae2f,0x00000000 
         long      0x3ffe0000,0x8a42f870,0x5669db46,0x00000000 
         long      0x3ffe0000,0x9db4224f,0xffe1157c,0x00000000 
         long      0x3ffe0000,0x891ac73a,0xe9819b50,0x00000000 
         long      0x3ffe0000,0x9fdadc26,0x8b7a12da,0x00000000 
         long      0x3ffe0000,0x87f78087,0xf78087f8,0x00000000 
         long      0x3ffe0000,0xa1fcff17,0xce733bd4,0x00000000 
         long      0x3ffe0000,0x86d90544,0x7a34acc6,0x00000000 
         long      0x3ffe0000,0xa41a9e8f,0x5446fb9f,0x00000000 
         long      0x3ffe0000,0x85bf3761,0x2cee3c9b,0x00000000 
         long      0x3ffe0000,0xa633cd7e,0x6771cd8b,0x00000000 
         long      0x3ffe0000,0x84a9f9c8,0x084a9f9d,0x00000000 
         long      0x3ffe0000,0xa8489e60,0x0b435a5e,0x00000000 
         long      0x3ffe0000,0x83993052,0x3fbe3368,0x00000000 
         long      0x3ffe0000,0xaa59233c,0xcca4bd49,0x00000000 
         long      0x3ffe0000,0x828cbfbe,0xb9a020a3,0x00000000 
         long      0x3ffe0000,0xac656dae,0x6bcc4985,0x00000000 
         long      0x3ffe0000,0x81848da8,0xfaf0d277,0x00000000 
         long      0x3ffe0000,0xae6d8ee3,0x60bb2468,0x00000000 
         long      0x3ffe0000,0x80808080,0x80808081,0x00000000 
         long      0x3ffe0000,0xb07197a2,0x3c46c654,0x00000000 
                                    
         set       adjk,l_scr1      
                                    
         set       x,fp_scr1        
         set       xdcare,x+2       
         set       xfrac,x+4        
                                    
         set       f,fp_scr2        
         set       ffrac,f+4        
                                    
         set       klog2,fp_scr3    
                                    
         set       saveu,fp_scr4    
                                    
                                    
                                    
                                    
                                    
                                    
         global    slognd           
slognd:                             
#--ENTRY POINT FOR LOG(X) FOR DENORMALIZED INPUT
                                    
         move.l    &-100,adjk(%a6)  # INPUT = 2^(ADJK) * FP0
                                    
#----normalize the input value by left shifting k bits (k to be determined
#----below), adjusting exponent and storing -k to  ADJK
#----the value TWOTO100 is no longer needed.
#----Note that this code assumes the denormalized input is NON-ZERO.
                                    
         movem.l   %d2-%d7,-(%a7)   # save some registers 
         move.l    &0x00000000,%d3  # D3 is exponent of smallest norm. #
         move.l    4(%a0),%d4       
         move.l    8(%a0),%d5       # (D4,D5) is (Hi_X,Lo_X)
         clr.l     %d2              # D2 used for holding K
                                    
         tst.l     %d4              
         bne.b     hix_not0         
                                    
hix_0:                              
         move.l    %d5,%d4          
         clr.l     %d5              
         move.l    &32,%d2          
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  
         lsl.l     %d6,%d4          
         add.l     %d6,%d2          # (D3,D4,D5) is normalized
                                    
         move.l    %d3,x(%a6)       
         move.l    %d4,xfrac(%a6)   
         move.l    %d5,xfrac+4(%a6) 
         neg.l     %d2              
         move.l    %d2,adjk(%a6)    
         fmove.x   x(%a6),%fp0      
         movem.l   (%a7)+,%d2-%d7   # restore registers
         lea       x(%a6),%a0       
         bra.b     logbgn           # begin regular log(X)
                                    
                                    
hix_not0:                            
         clr.l     %d6              
         bfffo     %d4{&0:&32},%d6  # find first 1
         move.l    %d6,%d2          # get k
         lsl.l     %d6,%d4          
         move.l    %d5,%d7          # a copy of D5
         lsl.l     %d6,%d5          
         neg.l     %d6              
         addi.l    &32,%d6          
         lsr.l     %d6,%d7          
         or.l      %d7,%d4          # (D3,D4,D5) normalized
                                    
         move.l    %d3,x(%a6)       
         move.l    %d4,xfrac(%a6)   
         move.l    %d5,xfrac+4(%a6) 
         neg.l     %d2              
         move.l    %d2,adjk(%a6)    
         fmove.x   x(%a6),%fp0      
         movem.l   (%a7)+,%d2-%d7   # restore registers
         lea       x(%a6),%a0       
         bra.b     logbgn           # begin regular log(X)
                                    
                                    
         global    slogn            
slogn:                              
#--ENTRY POINT FOR LOG(X) FOR X FINITE, NON-ZERO, NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
         move.l    &0x00000000,adjk(%a6) 
                                    
logbgn:                             
#--FPCR SAVED AND CLEARED, INPUT IS 2^(ADJK)*FP0, FP0 CONTAINS
#--A FINITE, NON-ZERO, NORMALIZED NUMBER.
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
                                    
         move.l    (%a0),x(%a6)     
         move.l    4(%a0),x+4(%a6)  
         move.l    8(%a0),x+8(%a6)  
                                    
         cmpi.l    %d0,&0           # CHECK IF X IS NEGATIVE
         blt.w     logneg           # LOG OF NEGATIVE ARGUMENT IS INVALID
         cmp2.l    %d0,bounds1      # X IS POSITIVE, CHECK IF X IS NEAR 1
         bcc.w     lognear1         # BOUNDS IS ROUGHLY [15/16, 17/16]
                                    
logmain:                            
#--THIS SHOULD BE THE USUAL CASE, X NOT VERY CLOSE TO 1
                                    
#--X = 2^(K) * Y, 1 <= Y < 2. THUS, Y = 1.XXXXXXXX....XX IN BINARY.
#--WE DEFINE F = 1.XXXXXX1, I.E. FIRST 7 BITS OF Y AND ATTACH A 1.
#--THE IDEA IS THAT LOG(X) = K*LOG2 + LOG(Y)
#--			 = K*LOG2 + LOG(F) + LOG(1 + (Y-F)/F).
#--NOTE THAT U = (Y-F)/F IS VERY SMALL AND THUS APPROXIMATING
#--LOG(1+U) CAN BE VERY EFFICIENT.
#--ALSO NOTE THAT THE VALUE 1/F IS STORED IN A TABLE SO THAT NO
#--DIVISION IS NEEDED TO CALCULATE (Y-F)/F. 
                                    
#--GET K, Y, F, AND ADDRESS OF 1/F.
         asr.l     &8,%d0           
         asr.l     &8,%d0           # SHIFTED 16 BITS, BIASED EXPO. OF X
         subi.l    &0x3fff,%d0      # THIS IS K
         add.l     adjk(%a6),%d0    # ADJUST K, ORIGINAL INPUT MAY BE  DENORM.
         lea       logtbl,%a0       # BASE ADDRESS OF 1/F AND LOG(F)
         fmove.l   %d0,%fp1         # CONVERT K TO FLOATING-POINT FORMAT
                                    
#--WHILE THE CONVERSION IS GOING ON, WE GET F AND ADDRESS OF 1/F
         move.l    &0x3fff0000,x(%a6) # X IS NOW Y, I.E. 2^(-K)*X
         move.l    xfrac(%a6),ffrac(%a6) 
         andi.l    &0xfe000000,ffrac(%a6) # FIRST 7 BITS OF Y
         ori.l     &0x01000000,ffrac(%a6) # GET F: ATTACH A 1 AT THE EIGHTH BIT
         move.l    ffrac(%a6),%d0   # READY TO GET ADDRESS OF 1/F
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           # SHIFTED 20, D0 IS THE DISPLACEMENT
         adda.l    %d0,%a0          # A0 IS THE ADDRESS FOR 1/F
                                    
         fmove.x   x(%a6),%fp0      
         move.l    &0x3fff0000,f(%a6) 
         clr.l     f+8(%a6)         
         fsub.x    f(%a6),%fp0      # Y-F
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2 WHILE FP0 IS NOT READY
#--SUMMARY: FP0 IS Y-F, A0 IS ADDRESS OF 1/F, FP1 IS K
#--REGISTERS SAVED: FPCR, FP1, FP2
                                    
lp1cont1:                            
#--AN RE-ENTRY POINT FOR LOGNP1
         fmul.x    (%a0),%fp0       # FP0 IS U = (Y-F)/F
         fmul.x    logof2,%fp1      # GET K*LOG2 WHILE FP0 IS NOT READY
         fmove.x   %fp0,%fp2        
         fmul.x    %fp2,%fp2        # FP2 IS V=U*U
         fmove.x   %fp1,klog2(%a6)  # PUT K*LOG2 IN MEMEORY, FREE FP1
                                    
#--LOG(1+U) IS APPROXIMATED BY
#--U + V*(A1+U*(A2+U*(A3+U*(A4+U*(A5+U*A6))))) WHICH IS
#--[U + V*(A1+V*(A3+V*A5))]  +  [U*V*(A2+V*(A4+V*A6))]
                                    
         fmove.x   %fp2,%fp3        
         fmove.x   %fp2,%fp1        
                                    
         fmul.d    loga6,%fp1       # V*A6
         fmul.d    loga5,%fp2       # V*A5
                                    
         fadd.d    loga4,%fp1       # A4+V*A6
         fadd.d    loga3,%fp2       # A3+V*A5
                                    
         fmul.x    %fp3,%fp1        # V*(A4+V*A6)
         fmul.x    %fp3,%fp2        # V*(A3+V*A5)
                                    
         fadd.d    loga2,%fp1       # A2+V*(A4+V*A6)
         fadd.d    loga1,%fp2       # A1+V*(A3+V*A5)
                                    
         fmul.x    %fp3,%fp1        # V*(A2+V*(A4+V*A6))
         adda.l    &16,%a0          # ADDRESS OF LOG(F)
         fmul.x    %fp3,%fp2        # V*(A1+V*(A3+V*A5)), FP3 RELEASED
                                    
         fmul.x    %fp0,%fp1        # U*V*(A2+V*(A4+V*A6))
         fadd.x    %fp2,%fp0        # U+V*(A1+V*(A3+V*A5)), FP2 RELEASED
                                    
         fadd.x    (%a0),%fp1       # LOG(F)+U*V*(A2+V*(A4+V*A6))
         fmovem.x  (%sp)+,%fp2/%fp3 # RESTORE FP2
         fadd.x    %fp1,%fp0        # FP0 IS LOG(F) + LOG(1+U)
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    klog2(%a6),%fp0  # FINAL ADD
         bra.l     t_frcinx         
                                    
                                    
lognear1:                            
#--REGISTERS SAVED: FPCR, FP1. FP0 CONTAINS THE INPUT.
         fmove.x   %fp0,%fp1        
         fsub.s    one,%fp1         # FP1 IS X-1
         fadd.s    one,%fp0         # FP0 IS X+1
         fadd.x    %fp1,%fp1        # FP1 IS 2(X-1)
#--LOG(X) = LOG(1+U/2)-LOG(1-U/2) WHICH IS AN ODD POLYNOMIAL
#--IN U, U = 2(X-1)/(X+1) = FP1/FP0
                                    
lp1cont2:                            
#--THIS IS AN RE-ENTRY POINT FOR LOGNP1
         fdiv.x    %fp0,%fp1        # FP1 IS U
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2
#--REGISTERS SAVED ARE NOW FPCR,FP1,FP2,FP3
#--LET V=U*U, W=V*V, CALCULATE
#--U + U*V*(B1 + V*(B2 + V*(B3 + V*(B4 + V*B5)))) BY
#--U + U*V*(  [B1 + W*(B3 + W*B5)]  +  [V*(B2 + W*B4)]  )
         fmove.x   %fp1,%fp0        
         fmul.x    %fp0,%fp0        # FP0 IS V
         fmove.x   %fp1,saveu(%a6)  # STORE U IN MEMORY, FREE FP1
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS W
                                    
         fmove.d   logb5,%fp3       
         fmove.d   logb4,%fp2       
                                    
         fmul.x    %fp1,%fp3        # W*B5
         fmul.x    %fp1,%fp2        # W*B4
                                    
         fadd.d    logb3,%fp3       # B3+W*B5
         fadd.d    logb2,%fp2       # B2+W*B4
                                    
         fmul.x    %fp3,%fp1        # W*(B3+W*B5), FP3 RELEASED
                                    
         fmul.x    %fp0,%fp2        # V*(B2+W*B4)
                                    
         fadd.d    logb1,%fp1       # B1+W*(B3+W*B5)
         fmul.x    saveu(%a6),%fp0  # FP0 IS U*V
                                    
         fadd.x    %fp2,%fp1        # B1+W*(B3+W*B5) + V*(B2+W*B4), FP2 RELEASED
         fmovem.x  (%sp)+,%fp2/%fp3 # FP2 RESTORED
                                    
         fmul.x    %fp1,%fp0        # U*V*( [B1+W*(B3+W*B5)] + [V*(B2+W*B4)] )
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    saveu(%a6),%fp0  
         bra.l     t_frcinx         
         rts                        
                                    
logneg:                             
#--REGISTERS SAVED FPCR. LOG(-VE) IS INVALID
         bra.l     t_operr          
                                    
         global    slognp1d         
slognp1d:                            
#--ENTRY POINT FOR LOG(1+Z) FOR DENORMALIZED INPUT
# Simply return the denorm
                                    
         bra.l     t_extdnrm        
                                    
         global    slognp1          
slognp1:                            
#--ENTRY POINT FOR LOG(1+X) FOR X FINITE, NON-ZERO, NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
         fabs.x    %fp0             # test magnitude
         fcmp.x    %fp0,lthold      # compare with min threshold
         fbgt.w    lp1real          # if greater, continue
         fmove.l   &0,%fpsr         # clr N flag from compare
         fmove.l   %d1,%fpcr        
         fmove.x   (%a0),%fp0       # return signed argument
         bra.l     t_frcinx         
                                    
lp1real:                            
         fmove.x   (%a0),%fp0       # LOAD INPUT
         move.l    &0x00000000,adjk(%a6) 
         fmove.x   %fp0,%fp1        # FP1 IS INPUT Z
         fadd.s    one,%fp0         # X := ROUND(1+Z)
         fmove.x   %fp0,x(%a6)      
         move.w    xfrac(%a6),xdcare(%a6) 
         move.l    x(%a6),%d0       
         cmpi.l    %d0,&0           
         ble.w     lp1neg0          # LOG OF ZERO OR -VE
         cmp2.l    %d0,bounds2      
         bcs.w     logmain          # BOUNDS2 IS [1/2,3/2]
#--IF 1+Z > 3/2 OR 1+Z < 1/2, THEN X, WHICH IS ROUNDING 1+Z,
#--CONTAINS AT LEAST 63 BITS OF INFORMATION OF Z. IN THAT CASE,
#--SIMPLY INVOKE LOG(X) FOR LOG(1+Z).
                                    
lp1near1:                            
#--NEXT SEE IF EXP(-1/16) < X < EXP(1/16)
         cmp2.l    %d0,bounds1      
         bcs.b     lp1care          
                                    
lp1one16:                            
#--EXP(-1/16) < X < EXP(1/16). LOG(1+Z) = LOG(1+U/2) - LOG(1-U/2)
#--WHERE U = 2Z/(2+Z) = 2Z/(1+X).
         fadd.x    %fp1,%fp1        # FP1 IS 2Z
         fadd.s    one,%fp0         # FP0 IS 1+X
#--U = FP1/FP0
         bra.w     lp1cont2         
                                    
lp1care:                            
#--HERE WE USE THE USUAL TABLE DRIVEN APPROACH. CARE HAS TO BE
#--TAKEN BECAUSE 1+Z CAN HAVE 67 BITS OF INFORMATION AND WE MUST
#--PRESERVE ALL THE INFORMATION. BECAUSE 1+Z IS IN [1/2,3/2],
#--THERE ARE ONLY TWO CASES.
#--CASE 1: 1+Z < 1, THEN K = -1 AND Y-F = (2-F) + 2Z
#--CASE 2: 1+Z > 1, THEN K = 0  AND Y-F = (1-F) + Z
#--ON RETURNING TO LP1CONT1, WE MUST HAVE K IN FP1, ADDRESS OF
#--(1/F) IN A0, Y-F IN FP0, AND FP2 SAVED.
                                    
         move.l    xfrac(%a6),ffrac(%a6) 
         andi.l    &0xfe000000,ffrac(%a6) 
         ori.l     &0x01000000,ffrac(%a6) # F OBTAINED
         cmpi.l    %d0,&0x3fff8000  # SEE IF 1+Z > 1
         bge.b     kiszero          
                                    
kisneg1:                            
         fmove.s   two,%fp0         
         move.l    &0x3fff0000,f(%a6) 
         clr.l     f+8(%a6)         
         fsub.x    f(%a6),%fp0      # 2-F
         move.l    ffrac(%a6),%d0   
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           # D0 CONTAINS DISPLACEMENT FOR 1/F
         fadd.x    %fp1,%fp1        # GET 2Z
         fmovem.x  %fp2/%fp3,-(%sp) # SAVE FP2 
         fadd.x    %fp1,%fp0        # FP0 IS Y-F = (2-F)+2Z
         lea       logtbl,%a0       # A0 IS ADDRESS OF 1/F
         adda.l    %d0,%a0          
         fmove.s   negone,%fp1      # FP1 IS K = -1
         bra.w     lp1cont1         
                                    
kiszero:                            
         fmove.s   one,%fp0         
         move.l    &0x3fff0000,f(%a6) 
         clr.l     f+8(%a6)         
         fsub.x    f(%a6),%fp0      # 1-F
         move.l    ffrac(%a6),%d0   
         andi.l    &0x7e000000,%d0  
         asr.l     &8,%d0           
         asr.l     &8,%d0           
         asr.l     &4,%d0           
         fadd.x    %fp1,%fp0        # FP0 IS Y-F
         fmovem.x  %fp2/%fp3,-(%sp) # FP2 SAVED
         lea       logtbl,%a0       
         adda.l    %d0,%a0          # A0 IS ADDRESS OF 1/F
         fmove.s   zero,%fp1        # FP1 IS K = 0
         bra.w     lp1cont1         
                                    
lp1neg0:                            
#--FPCR SAVED. D0 IS X IN COMPACT FORM.
         cmpi.l    %d0,&0           
         blt.b     lp1neg           
lp1zero:                            
         fmove.s   negone,%fp0      
                                    
         fmove.l   %d1,%fpcr        
         bra.l     t_dz             
                                    
lp1neg:                             
         fmove.s   zero,%fp0        
                                    
         fmove.l   %d1,%fpcr        
         bra.l     t_operr          
                                    
                                    
	version 3
