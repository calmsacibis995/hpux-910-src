ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/satan.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:50:26 $
                                    
#
#	satan.sa 3.3 12/19/90
#
#	The entry point satan computes the arctagent of an
#	input value. satand does the same except the input value is a
#	denormalized number.
#
#	Input: Double-extended value in memory location pointed to by address
#		register a0.
#
#	Output:	Arctan(X) returned in floating-point register Fp0.
#
#	Accuracy and Monotonicity: The returned result is within 2 ulps in
#		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
#		result is subsequently rounded to double precision. The
#		result is provably monotonic in double precision.
#
#	Speed: The program satan takes approximately 160 cycles for input
#		argument X such that 1/16 < |X| < 16. For the other arguments,
#		the program will run no worse than 10% slower.
#
#	Algorithm:
#	Step 1. If |X| >= 16 or |X| < 1/16, go to Step 5.
#
#	Step 2. Let X = sgn * 2**k * 1.xxxxxxxx...x. Note that k = -4, -3,..., or 3.
#		Define F = sgn * 2**k * 1.xxxx1, i.e. the first 5 significant bits
#		of X with a bit-1 attached at the 6-th bit position. Define u
#		to be u = (X-F) / (1 + X*F).
#
#	Step 3. Approximate arctan(u) by a polynomial poly.
#
#	Step 4. Return arctan(F) + poly, arctan(F) is fetched from a table of values
#		calculated beforehand. Exit.
#
#	Step 5. If |X| >= 16, go to Step 7.
#
#	Step 6. Approximate arctan(X) by an odd polynomial in X. Exit.
#
#	Step 7. Define X' = -1/X. Approximate arctan(X') by an odd polynomial in X'.
#		Arctan(X) = sign(X)*Pi/2 + arctan(X'). Exit.
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
bounds1: long      0x3ffb8000,0x4002ffff 
                                    
one:     long      0x3f800000       
                                    
         long      0x00000000       
                                    
atana3:  long      0xbff6687e,0x314987d8 
atana2:  long      0x4002ac69,0x34a26db3 
                                    
atana1:  long      0xbfc2476f,0x4e1da28e 
atanb6:  long      0x3fb34444,0x7f876989 
                                    
atanb5:  long      0xbfb744ee,0x7faf45db 
atanb4:  long      0x3fbc71c6,0x46940220 
                                    
atanb3:  long      0xbfc24924,0x921872f9 
atanb2:  long      0x3fc99999,0x99998fa9 
                                    
atanb1:  long      0xbfd55555,0x55555555 
atanc5:  long      0xbfb70bf3,0x98539e6a 
                                    
atanc4:  long      0x3fbc7187,0x962d1d7d 
atanc3:  long      0xbfc24924,0x827107b8 
                                    
atanc2:  long      0x3fc99999,0x9996263e 
atanc1:  long      0xbfd55555,0x55555536 
                                    
ppiby2:  long      0x3fff0000,0xc90fdaa2,0x2168c235,0x00000000 
npiby2:  long      0xbfff0000,0xc90fdaa2,0x2168c235,0x00000000 
ptiny:   long      0x00010000,0x80000000,0x00000000,0x00000000 
ntiny:   long      0x80010000,0x80000000,0x00000000,0x00000000 
                                    
atantbl:                            
         long      0x3ffb0000,0x83d152c5,0x060b7a51,0x00000000 
         long      0x3ffb0000,0x8bc85445,0x65498b8b,0x00000000 
         long      0x3ffb0000,0x93be4060,0x17626b0d,0x00000000 
         long      0x3ffb0000,0x9bb3078d,0x35aec202,0x00000000 
         long      0x3ffb0000,0xa3a69a52,0x5ddce7de,0x00000000 
         long      0x3ffb0000,0xab98e943,0x62765619,0x00000000 
         long      0x3ffb0000,0xb389e502,0xf9c59862,0x00000000 
         long      0x3ffb0000,0xbb797e43,0x6b09e6fb,0x00000000 
         long      0x3ffb0000,0xc367a5c7,0x39e5f446,0x00000000 
         long      0x3ffb0000,0xcb544c61,0xcff7d5c6,0x00000000 
         long      0x3ffb0000,0xd33f62f8,0x2488533e,0x00000000 
         long      0x3ffb0000,0xdb28da81,0x62404c77,0x00000000 
         long      0x3ffb0000,0xe310a407,0x8ad34f18,0x00000000 
         long      0x3ffb0000,0xeaf6b0a8,0x188ee1eb,0x00000000 
         long      0x3ffb0000,0xf2daf194,0x9dbe79d5,0x00000000 
         long      0x3ffb0000,0xfabd5813,0x61d47e3e,0x00000000 
         long      0x3ffc0000,0x8346ac21,0x0959ecc4,0x00000000 
         long      0x3ffc0000,0x8b232a08,0x304282d8,0x00000000 
         long      0x3ffc0000,0x92fb70b8,0xd29ae2f9,0x00000000 
         long      0x3ffc0000,0x9acf476f,0x5ccd1cb4,0x00000000 
         long      0x3ffc0000,0xa29e7630,0x4954f23f,0x00000000 
         long      0x3ffc0000,0xaa68c5d0,0x8ab85230,0x00000000 
         long      0x3ffc0000,0xb22dfffd,0x9d539f83,0x00000000 
         long      0x3ffc0000,0xb9edef45,0x3e900ea5,0x00000000 
         long      0x3ffc0000,0xc1a85f1c,0xc75e3ea5,0x00000000 
         long      0x3ffc0000,0xc95d1be8,0x28138de6,0x00000000 
         long      0x3ffc0000,0xd10bf300,0x840d2de4,0x00000000 
         long      0x3ffc0000,0xd8b4b2ba,0x6bc05e7a,0x00000000 
         long      0x3ffc0000,0xe0572a6b,0xb42335f6,0x00000000 
         long      0x3ffc0000,0xe7f32a70,0xea9caa8f,0x00000000 
         long      0x3ffc0000,0xef888432,0x64ecefaa,0x00000000 
         long      0x3ffc0000,0xf7170a28,0xecc06666,0x00000000 
         long      0x3ffd0000,0x812fd288,0x332dad32,0x00000000 
         long      0x3ffd0000,0x88a8d1b1,0x218e4d64,0x00000000 
         long      0x3ffd0000,0x9012ab3f,0x23e4aee8,0x00000000 
         long      0x3ffd0000,0x976cc3d4,0x11e7f1b9,0x00000000 
         long      0x3ffd0000,0x9eb68949,0x3889a227,0x00000000 
         long      0x3ffd0000,0xa5ef72c3,0x4487361b,0x00000000 
         long      0x3ffd0000,0xad1700ba,0xf07a7227,0x00000000 
         long      0x3ffd0000,0xb42cbcfa,0xfd37efb7,0x00000000 
         long      0x3ffd0000,0xbb303a94,0x0ba80f89,0x00000000 
         long      0x3ffd0000,0xc22115c6,0xfcaebbaf,0x00000000 
         long      0x3ffd0000,0xc8fef3e6,0x86331221,0x00000000 
         long      0x3ffd0000,0xcfc98330,0xb4000c70,0x00000000 
         long      0x3ffd0000,0xd6807aa1,0x102c5bf9,0x00000000 
         long      0x3ffd0000,0xdd2399bc,0x31252aa3,0x00000000 
         long      0x3ffd0000,0xe3b2a855,0x6b8fc517,0x00000000 
         long      0x3ffd0000,0xea2d764f,0x64315989,0x00000000 
         long      0x3ffd0000,0xf3bf5bf8,0xbad1a21d,0x00000000 
         long      0x3ffe0000,0x801ce39e,0x0d205c9a,0x00000000 
         long      0x3ffe0000,0x8630a2da,0xda1ed066,0x00000000 
         long      0x3ffe0000,0x8c1ad445,0xf3e09b8c,0x00000000 
         long      0x3ffe0000,0x91db8f16,0x64f350e2,0x00000000 
         long      0x3ffe0000,0x97731420,0x365e538c,0x00000000 
         long      0x3ffe0000,0x9ce1c8e6,0xa0b8cdba,0x00000000 
         long      0x3ffe0000,0xa22832db,0xcadaae09,0x00000000 
         long      0x3ffe0000,0xa746f2dd,0xb7602294,0x00000000 
         long      0x3ffe0000,0xac3ec0fb,0x997dd6a2,0x00000000 
         long      0x3ffe0000,0xb110688a,0xebdc6f6a,0x00000000 
         long      0x3ffe0000,0xb5bcc490,0x59ecc4b0,0x00000000 
         long      0x3ffe0000,0xba44bc7d,0xd470782f,0x00000000 
         long      0x3ffe0000,0xbea94144,0xfd049aac,0x00000000 
         long      0x3ffe0000,0xc2eb4abb,0x661628b6,0x00000000 
         long      0x3ffe0000,0xc70bd54c,0xe602ee14,0x00000000 
         long      0x3ffe0000,0xcd000549,0xadec7159,0x00000000 
         long      0x3ffe0000,0xd48457d2,0xd8ea4ea3,0x00000000 
         long      0x3ffe0000,0xdb948da7,0x12dece3b,0x00000000 
         long      0x3ffe0000,0xe23855f9,0x69e8096a,0x00000000 
         long      0x3ffe0000,0xe8771129,0xc4353259,0x00000000 
         long      0x3ffe0000,0xee57c16e,0x0d379c0d,0x00000000 
         long      0x3ffe0000,0xf3e10211,0xa87c3779,0x00000000 
         long      0x3ffe0000,0xf919039d,0x758b8d41,0x00000000 
         long      0x3ffe0000,0xfe058b8f,0x64935fb3,0x00000000 
         long      0x3fff0000,0x8155fb49,0x7b685d04,0x00000000 
         long      0x3fff0000,0x83889e35,0x49d108e1,0x00000000 
         long      0x3fff0000,0x859cfa76,0x511d724b,0x00000000 
         long      0x3fff0000,0x87952ecf,0xff8131e7,0x00000000 
         long      0x3fff0000,0x89732fd1,0x9557641b,0x00000000 
         long      0x3fff0000,0x8b38cad1,0x01932a35,0x00000000 
         long      0x3fff0000,0x8ce7a8d8,0x301ee6b5,0x00000000 
         long      0x3fff0000,0x8f46a39e,0x2eae5281,0x00000000 
         long      0x3fff0000,0x922da7d7,0x91888487,0x00000000 
         long      0x3fff0000,0x94d19fcb,0xdedf5241,0x00000000 
         long      0x3fff0000,0x973ab944,0x19d2a08b,0x00000000 
         long      0x3fff0000,0x996ff00e,0x08e10b96,0x00000000 
         long      0x3fff0000,0x9b773f95,0x12321da7,0x00000000 
         long      0x3fff0000,0x9d55cc32,0x0f935624,0x00000000 
         long      0x3fff0000,0x9f100575,0x006cc571,0x00000000 
         long      0x3fff0000,0xa0a9c290,0xd97cc06c,0x00000000 
         long      0x3fff0000,0xa22659eb,0xebc0630a,0x00000000 
         long      0x3fff0000,0xa388b4af,0xf6ef0ec9,0x00000000 
         long      0x3fff0000,0xa4d35f10,0x61d292c4,0x00000000 
         long      0x3fff0000,0xa60895dc,0xfbe3187e,0x00000000 
         long      0x3fff0000,0xa72a51dc,0x7367beac,0x00000000 
         long      0x3fff0000,0xa83a5153,0x0956168f,0x00000000 
         long      0x3fff0000,0xa93a2007,0x7539546e,0x00000000 
         long      0x3fff0000,0xaa9e7245,0x023b2605,0x00000000 
         long      0x3fff0000,0xac4c84ba,0x6fe4d58f,0x00000000 
         long      0x3fff0000,0xadce4a4a,0x606b9712,0x00000000 
         long      0x3fff0000,0xaf2a2dcd,0x8d263c9c,0x00000000 
         long      0x3fff0000,0xb0656f81,0xf22265c7,0x00000000 
         long      0x3fff0000,0xb1846515,0x0f71496a,0x00000000 
         long      0x3fff0000,0xb28aaa15,0x6f9ada35,0x00000000 
         long      0x3fff0000,0xb37b44ff,0x3766b895,0x00000000 
         long      0x3fff0000,0xb458c3dc,0xe9630433,0x00000000 
         long      0x3fff0000,0xb525529d,0x562246bd,0x00000000 
         long      0x3fff0000,0xb5e2cca9,0x5f9d88cc,0x00000000 
         long      0x3fff0000,0xb692cada,0x7aca1ada,0x00000000 
         long      0x3fff0000,0xb736aea7,0xa6925838,0x00000000 
         long      0x3fff0000,0xb7cfab28,0x7e9f7b36,0x00000000 
         long      0x3fff0000,0xb85ecc66,0xcb219835,0x00000000 
         long      0x3fff0000,0xb8e4fd5a,0x20a593da,0x00000000 
         long      0x3fff0000,0xb99f41f6,0x4aff9bb5,0x00000000 
         long      0x3fff0000,0xba7f1e17,0x842bbe7b,0x00000000 
         long      0x3fff0000,0xbb471285,0x7637e17d,0x00000000 
         long      0x3fff0000,0xbbfabe8a,0x4788df6f,0x00000000 
         long      0x3fff0000,0xbc9d0fad,0x2b689d79,0x00000000 
         long      0x3fff0000,0xbd306a39,0x471ecd86,0x00000000 
         long      0x3fff0000,0xbdb6c731,0x856af18a,0x00000000 
         long      0x3fff0000,0xbe31cac5,0x02e80d70,0x00000000 
         long      0x3fff0000,0xbea2d55c,0xe33194e2,0x00000000 
         long      0x3fff0000,0xbf0b10b7,0xc03128f0,0x00000000 
         long      0x3fff0000,0xbf6b7a18,0xdacb778d,0x00000000 
         long      0x3fff0000,0xbfc4ea46,0x63fa18f6,0x00000000 
         long      0x3fff0000,0xc0181bde,0x8b89a454,0x00000000 
         long      0x3fff0000,0xc065b066,0xcfbf6439,0x00000000 
         long      0x3fff0000,0xc0ae345f,0x56340ae6,0x00000000 
         long      0x3fff0000,0xc0f22291,0x9cb9e6a7,0x00000000 
                                    
         set       x,fp_scr1        
         set       xdcare,x+2       
         set       xfrac,x+4        
         set       xfraclo,x+8      
                                    
         set       atanf,fp_scr2    
         set       atanfhi,atanf+4  
         set       atanflo,atanf+8  
                                    
                                    
                                    
                                    
                                    
         global    satand           
satand:                             
#--ENTRY POINT FOR ATAN(X) FOR DENORMALIZED ARGUMENT
                                    
         bra.l     t_extdnrm        
                                    
         global    satan            
satan:                              
#--ENTRY POINT FOR ATAN(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
                                    
         fmove.x   (%a0),%fp0       # LOAD INPUT
                                    
         move.l    (%a0),%d0        
         move.w    4(%a0),%d0       
         fmove.x   %fp0,x(%a6)      
         andi.l    &0x7fffffff,%d0  
                                    
         cmpi.l    %d0,&0x3ffb8000  # |X| >= 1/16?
         bge.b     atanok1          
         bra.w     atansm           
                                    
atanok1:                            
         cmpi.l    %d0,&0x4002ffff  # |X| < 16 ?
         ble.b     atanmain         
         bra.w     atanbig          
                                    
                                    
#--THE MOST LIKELY CASE, |X| IN [1/16, 16). WE USE TABLE TECHNIQUE
#--THE IDEA IS ATAN(X) = ATAN(F) + ATAN( [X-F] / [1+XF] ).
#--SO IF F IS CHOSEN TO BE CLOSE TO X AND ATAN(F) IS STORED IN
#--A TABLE, ALL WE NEED IS TO APPROXIMATE ATAN(U) WHERE
#--U = (X-F)/(1+XF) IS SMALL (REMEMBER F IS CLOSE TO X). IT IS
#--TRUE THAT A DIVIDE IS NOW NEEDED, BUT THE APPROXIMATION FOR
#--ATAN(U) IS A VERY SHORT POLYNOMIAL AND THE INDEXING TO
#--FETCH F AND SAVING OF REGISTERS CAN BE ALL HIDED UNDER THE
#--DIVIDE. IN THE END THIS METHOD IS MUCH FASTER THAN A TRADITIONAL
#--ONE. NOTE ALSO THAT THE TRADITIONAL SCHEME THAT APPROXIMATE
#--ATAN(X) DIRECTLY WILL NEED TO USE A RATIONAL APPROXIMATION
#--(DIVISION NEEDED) ANYWAY BECAUSE A POLYNOMIAL APPROXIMATION
#--WILL INVOLVE A VERY LONG POLYNOMIAL.
                                    
#--NOW WE SEE X AS +-2^K * 1.BBBBBBB....B <- 1. + 63 BITS
#--WE CHOSE F TO BE +-2^K * 1.BBBB1
#--THAT IS IT MATCHES THE EXPONENT AND FIRST 5 BITS OF X, THE
#--SIXTH BITS IS SET TO BE 1. SINCE K = -4, -3, ..., 3, THERE
#--ARE ONLY 8 TIMES 16 = 2^7 = 128 |F|'S. SINCE ATAN(-|F|) IS
#-- -ATAN(|F|), WE NEED TO STORE ONLY ATAN(|F|).
                                    
atanmain:                            
                                    
         move.w    &0x0000,xdcare(%a6) # CLEAN UP X JUST IN CASE
         andi.l    &0xf8000000,xfrac(%a6) # FIRST 5 BITS
         ori.l     &0x04000000,xfrac(%a6) # SET 6-TH BIT TO 1
         move.l    &0x00000000,xfraclo(%a6) # LOCATION OF X IS NOW F
                                    
         fmove.x   %fp0,%fp1        # FP1 IS X
         fmul.x    x(%a6),%fp1      # FP1 IS X*F, NOTE THAT X*F > 0
         fsub.x    x(%a6),%fp0      # FP0 IS X-F
         fadd.s    &0f1.0,%fp1      # FP1 IS 1 + X*F
         fdiv.x    %fp1,%fp0        # FP0 IS U = (X-F)/(1+X*F)
                                    
#--WHILE THE DIVISION IS TAKING ITS TIME, WE FETCH ATAN(|F|)
#--CREATE ATAN(F) AND STORE IT IN ATANF, AND
#--SAVE REGISTERS FP2.
                                    
         move.l    %d2,-(%a7)       # SAVE d2 TEMPORARILY
         move.l    %d0,%d2          # THE EXPO AND 16 BITS OF X
         andi.l    &0x00007800,%d0  # 4 VARYING BITS OF F'S FRACTION
         andi.l    &0x7fff0000,%d2  # EXPONENT OF F
         subi.l    &0x3ffb0000,%d2  # K+4
         asr.l     &1,%d2           
         add.l     %d2,%d0          # THE 7 BITS IDENTIFYING F
         asr.l     &7,%d0           # INDEX INTO TBL OF ATAN(|F|)
         lea       atantbl,%a1      
         adda.l    %d0,%a1          # ADDRESS OF ATAN(|F|)
         move.l    (%a1)+,atanf(%a6) 
         move.l    (%a1)+,atanfhi(%a6) 
         move.l    (%a1)+,atanflo(%a6) # ATANF IS NOW ATAN(|F|)
         move.l    x(%a6),%d0       # LOAD SIGN AND EXPO. AGAIN
         andi.l    &0x80000000,%d0  # SIGN(F)
         or.l      %d0,atanf(%a6)   # ATANF IS NOW SIGN(F)*ATAN(|F|)
         move.l    (%a7)+,%d2       # RESTORE d2
                                    
#--THAT'S ALL I HAVE TO DO FOR NOW,
#--BUT ALAS, THE DIVIDE IS STILL CRANKING!
                                    
#--U IN FP0, WE ARE NOW READY TO COMPUTE ATAN(U) AS
#--U + A1*U*V*(A2 + V*(A3 + V)), V = U*U
#--THE POLYNOMIAL MAY LOOK STRANGE, BUT IS NEVERTHELESS CORRECT.
#--THE NATURAL FORM IS U + U*V*(A1 + V*(A2 + V*A3))
#--WHAT WE HAVE HERE IS MERELY	A1 = A3, A2 = A1/A3, A3 = A2/A3.
#--THE REASON FOR THIS REARRANGEMENT IS TO MAKE THE INDEPENDENT
#--PARTS A1*U*V AND (A2 + ... STUFF) MORE LOAD-BALANCED
                                    
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        
         fmove.d   atana3,%fp2      
         fadd.x    %fp1,%fp2        # A3+V
         fmul.x    %fp1,%fp2        # V*(A3+V)
         fmul.x    %fp0,%fp1        # U*V
         fadd.d    atana2,%fp2      # A2+V*(A3+V)
         fmul.d    atana1,%fp1      # A1*U*V
         fmul.x    %fp2,%fp1        # A1*U*V*(A2+V*(A3+V))
                                    
         fadd.x    %fp1,%fp0        # ATAN(U), FP1 RELEASED
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    atanf(%a6),%fp0  # ATAN(X)
         bra.l     t_frcinx         
                                    
atanbors:                            
#--|X| IS IN d0 IN COMPACT FORM. FP1, d0 SAVED.
#--FP0 IS X AND |X| <= 1/16 OR |X| >= 16.
         cmpi.l    %d0,&0x3fff8000  
         bgt.w     atanbig          # I.E. |X| >= 16
                                    
atansm:                             
#--|X| <= 1/16
#--IF |X| < 2^(-40), RETURN X AS ANSWER. OTHERWISE, APPROXIMATE
#--ATAN(X) BY X + X*Y*(B1+Y*(B2+Y*(B3+Y*(B4+Y*(B5+Y*B6)))))
#--WHICH IS X + X*Y*( [B1+Z*(B3+Z*B5)] + [Y*(B2+Z*(B4+Z*B6)] )
#--WHERE Y = X*X, AND Z = Y*Y.
                                    
         cmpi.l    %d0,&0x3fd78000  
         blt.w     atantiny         
#--COMPUTE POLYNOMIAL
         fmul.x    %fp0,%fp0        # FP0 IS Y = X*X
                                    
                                    
         move.w    &0x0000,xdcare(%a6) 
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS Z = Y*Y
                                    
         fmove.d   atanb6,%fp2      
         fmove.d   atanb5,%fp3      
                                    
         fmul.x    %fp1,%fp2        # Z*B6
         fmul.x    %fp1,%fp3        # Z*B5
                                    
         fadd.d    atanb4,%fp2      # B4+Z*B6
         fadd.d    atanb3,%fp3      # B3+Z*B5
                                    
         fmul.x    %fp1,%fp2        # Z*(B4+Z*B6)
         fmul.x    %fp3,%fp1        # Z*(B3+Z*B5)
                                    
         fadd.d    atanb2,%fp2      # B2+Z*(B4+Z*B6)
         fadd.d    atanb1,%fp1      # B1+Z*(B3+Z*B5)
                                    
         fmul.x    %fp0,%fp2        # Y*(B2+Z*(B4+Z*B6))
         fmul.x    x(%a6),%fp0      # X*Y
                                    
         fadd.x    %fp2,%fp1        # [B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))]
                                    
                                    
         fmul.x    %fp1,%fp0        # X*Y*([B1+Z*(B3+Z*B5)]+[Y*(B2+Z*(B4+Z*B6))])
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fadd.x    x(%a6),%fp0      
                                    
         bra.l     t_frcinx         
                                    
atantiny:                            
#--|X| < 2^(-40), ATAN(X) = X
         move.w    &0x0000,xdcare(%a6) 
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
         fmove.x   x(%a6),%fp0      # last inst - possible exception set
                                    
         bra.l     t_frcinx         
                                    
atanbig:                            
#--IF |X| > 2^(100), RETURN	SIGN(X)*(PI/2 - TINY). OTHERWISE,
#--RETURN SIGN(X)*PI/2 + ATAN(-1/X).
         cmpi.l    %d0,&0x40638000  
         bgt.w     atanhuge         
                                    
#--APPROXIMATE ATAN(-1/X) BY
#--X'+X'*Y*(C1+Y*(C2+Y*(C3+Y*(C4+Y*C5)))), X' = -1/X, Y = X'*X'
#--THIS CAN BE RE-WRITTEN AS
#--X'+X'*Y*( [C1+Z*(C3+Z*C5)] + [Y*(C2+Z*C4)] ), Z = Y*Y.
                                    
         fmove.s   &0f-1.0,%fp1     # LOAD -1
         fdiv.x    %fp0,%fp1        # FP1 IS -1/X
                                    
                                    
#--DIVIDE IS STILL CRANKING
                                    
         fmove.x   %fp1,%fp0        # FP0 IS X'
         fmul.x    %fp0,%fp0        # FP0 IS Y = X__hpasm_string1
         fmove.x   %fp1,x(%a6)      # X IS REALLY X'
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # FP1 IS Z = Y*Y
                                    
         fmove.d   atanc5,%fp3      
         fmove.d   atanc4,%fp2      
                                    
         fmul.x    %fp1,%fp3        # Z*C5
         fmul.x    %fp1,%fp2        # Z*B4
                                    
         fadd.d    atanc3,%fp3      # C3+Z*C5
         fadd.d    atanc2,%fp2      # C2+Z*C4
                                    
         fmul.x    %fp3,%fp1        # Z*(C3+Z*C5), FP3 RELEASED
         fmul.x    %fp0,%fp2        # Y*(C2+Z*C4)
                                    
         fadd.d    atanc1,%fp1      # C1+Z*(C3+Z*C5)
         fmul.x    x(%a6),%fp0      # X'*Y
                                    
         fadd.x    %fp2,%fp1        # [Y*(C2+Z*C4)]+[C1+Z*(C3+Z*C5)]
                                    
                                    
         fmul.x    %fp1,%fp0        # X'*Y*([B1+Z*(B3+Z*B5)]
#					...	+[Y*(B2+Z*(B4+Z*B6))])
         fadd.x    x(%a6),%fp0      
                                    
         fmove.l   %d1,%fpcr        # restore users exceptions
                                    
         btst.b    &7,(%a0)         
         beq.b     pos_big          
                                    
neg_big:                            
         fadd.x    npiby2,%fp0      
         bra.l     t_frcinx         
                                    
pos_big:                            
         fadd.x    ppiby2,%fp0      
         bra.l     t_frcinx         
                                    
atanhuge:                            
#--RETURN SIGN(X)*(PIBY2 - TINY) = SIGN(X)*PIBY2 - SIGN(X)*TINY
         btst.b    &7,(%a0)         
         beq.b     pos_huge         
                                    
neg_huge:                            
         fmove.x   npiby2,%fp0      
         fmove.l   %d1,%fpcr        
         fsub.x    ntiny,%fp0       
         bra.l     t_frcinx         
                                    
pos_huge:                            
         fmove.x   ppiby2,%fp0      
         fmove.l   %d1,%fpcr        
         fsub.x    ptiny,%fp0       
         bra.l     t_frcinx         
                                    
                                    
	version 3
