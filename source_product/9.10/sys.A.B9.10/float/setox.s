ifdef(`ifndef',,`define(ifndef,`ifdef(`$1',`$3',`$2')')') # HPASM Generated
# $Source: /source/hpux_source/kernel/sys.SWT68K_300/float/RCS/setox.s,v $
# $Revision: 1.3.84.3 $      $Author: kcs $
# $State: Exp $         $Locker:  $
# $Date: 93/09/17 20:51:21 $
                                    
#
#	setox.sa 3.1 12/10/90
#
#	The entry point setox computes the exponential of a value.
#	setoxd does the same except the input value is a denormalized
#	number.	setoxm1 computes exp(X)-1, and setoxm1d computes
#	exp(X)-1 for denormalized X.
#
#	INPUT
#	-----
#	Double-extended value in memory location pointed to by address
#	register a0.
#
#	OUTPUT
#	------
#	exp(X) or exp(X)-1 returned in floating-point register fp0.
#
#	ACCURACY and MONOTONICITY
#	-------------------------
#	The returned result is within 0.85 ulps in 64 significant bit, i.e.
#	within 0.5001 ulp to 53 bits if the result is subsequently rounded
#	to double precision. The result is provably monotonic in double
#	precision.
#
#	SPEED
#	-----
#	Two timings are measured, both in the copy-back mode. The
#	first one is measured when the function is invoked the first time
#	(so the instructions and data are not in cache), and the
#	second one is measured when the function is reinvoked at the same
#	input argument.
#
#	The program setox takes approximately 210/190 cycles for input
#	argument X whose magnitude is less than 16380 log2, which
#	is the usual situation.	For the less common arguments,
#	depending on their values, the program may run faster or slower --
#	but no worse than 10% slower even in the extreme cases.
#
#	The program setoxm1 takes approximately ???/??? cycles for input
#	argument X, 0.25 <= |X| < 70log2. For |X| < 0.25, it takes
#	approximately ???/??? cycles. For the less common arguments,
#	depending on their values, the program may run faster or slower --
#	but no worse than 10% slower even in the extreme cases.
#
#	ALGORITHM and IMPLEMENTATION NOTES
#	----------------------------------
#
#	setoxd
#	------
#	Step 1.	Set ans := 1.0
#
#	Step 2.	Return	ans := ans + sign(X)*2^(-126). Exit.
#	Notes:	This will always generate one exception -- inexact.
#
#
#	setox
#	-----
#
#	Step 1.	Filter out extreme cases of input argument.
#		1.1	If |X| >= 2^(-65), go to Step 1.3.
#		1.2	Go to Step 7.
#		1.3	If |X| < 16380 log(2), go to Step 2.
#		1.4	Go to Step 8.
#	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
#		 To avoid the use of floating-point comparisons, a
#		 compact representation of |X| is used. This format is a
#		 32-bit integer, the upper (more significant) 16 bits are
#		 the sign and biased exponent field of |X|; the lower 16
#		 bits are the 16 most significant fraction (including the
#		 explicit bit) bits of |X|. Consequently, the comparisons
#		 in Steps 1.1 and 1.3 can be performed by integer comparison.
#		 Note also that the constant 16380 log(2) used in Step 1.3
#		 is also in the compact form. Thus taking the branch
#		 to Step 2 guarantees |X| < 16380 log(2). There is no harm
#		 to have a small number of cases where |X| is less than,
#		 but close to, 16380 log(2) and the branch to Step 9 is
#		 taken.
#
#	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
#		2.1	Set AdjFlag := 0 (indicates the branch 1.3 -> 2 was taken)
#		2.2	N := round-to-nearest-integer( X * 64/log2 ).
#		2.3	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
#		2.4	Calculate	M = (N - J)/64; so N = 64M + J.
#		2.5	Calculate the address of the stored value of 2^(J/64).
#		2.6	Create the value Scale = 2^M.
#	Notes:	The calculation in 2.2 is really performed by
#
#			Z := X * constant
#			N := round-to-nearest-integer(Z)
#
#		 where
#
#			constant := single-precision( 64/log 2 ).
#
#		 Using a single-precision constant avoids memory access.
#		 Another effect of using a single-precision "constant" is
#		 that the calculated value Z is
#
#			Z = X*(64/log2)*(1+eps), |eps| <= 2^(-24).
#
#		 This error has to be considered later in Steps 3 and 4.
#
#	Step 3.	Calculate X - N*log2/64.
#		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
#		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
#	Notes:	a) The way L1 and L2 are chosen ensures L1+L2 approximate
#		 the value	-log2/64	to 88 bits of accuracy.
#		 b) N*L1 is exact because N is no longer than 22 bits and
#		 L1 is no longer than 24 bits.
#		 c) The calculation X+N*L1 is also exact due to cancellation.
#		 Thus, R is practically X+N(L1+L2) to full 64 bits.
#		 d) It is important to estimate how large can |R| be after
#		 Step 3.2.
#
#			N = rnd-to-int( X*64/log2 (1+eps) ), |eps|<=2^(-24)
#			X*64/log2 (1+eps)	=	N + f,	|f| <= 0.5
#			X*64/log2 - N	=	f - eps*X 64/log2
#			X - N*log2/64	=	f*log2/64 - eps*X
#
#
#		 Now |X| <= 16446 log2, thus
#
#			|X - N*log2/64| <= (0.5 + 16446/2^(18))*log2/64
#					<= 0.57 log2/64.
#		 This bound will be used in Step 4.
#
#	Step 4.	Approximate exp(R)-1 by a polynomial
#			p = R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: A1 (which is 1/2), A4 and A5
#		 are single precision; A2 and A3 are double precision.
#		 b) Even with the restrictions above,
#			|p - (exp(R)-1)| < 2^(-68.8) for all |R| <= 0.0062.
#		 Note that 0.0062 is slightly bigger than 0.57 log2/64.
#		 c) To fully utilize the pipeline, p is separated into
#		 two independent pieces of roughly equal complexities
#			p = [ R + R*S*(A2 + S*A4) ]	+
#				[ S*(A1 + S*(A3 + S*A5)) ]
#		 where S = R*R.
#
#	Step 5.	Compute 2^(J/64)*exp(R) = 2^(J/64)*(1+p) by
#				ans := T + ( T*p + t)
#		 where T and t are the stored values for 2^(J/64).
#	Notes:	2^(J/64) is stored as T and t where T+t approximates
#		 2^(J/64) to roughly 85 bits; T is in extended precision
#		 and t is in single precision. Note also that T is rounded
#		 to 62 bits so that the last two bits of T are zero. The
#		 reason for such a special form is that T-1, T-2, and T-8
#		 will all be exact --- a property that will give much
#		 more accurate computation of the function EXPM1.
#
#	Step 6.	Reconstruction of exp(X)
#			exp(X) = 2^M * 2^(J/64) * exp(R).
#		6.1	If AdjFlag = 0, go to 6.3
#		6.2	ans := ans * AdjScale
#		6.3	Restore the user FPCR
#		6.4	Return ans := ans * Scale. Exit.
#	Notes:	If AdjFlag = 0, we have X = Mlog2 + Jlog2/64 + R,
#		 |M| <= 16380, and Scale = 2^M. Moreover, exp(X) will
#		 neither overflow nor underflow. If AdjFlag = 1, that
#		 means that
#			X = (M1+M)log2 + Jlog2/64 + R, |M1+M| >= 16380.
#		 Hence, exp(X) may overflow or underflow or neither.
#		 When that is the case, AdjScale = 2^(M1) where M1 is
#		 approximately M. Thus 6.2 will never cause over/underflow.
#		 Possible exception in 6.4 is overflow or underflow.
#		 The inexact exception is not generated in 6.4. Although
#		 one can argue that the inexact flag should always be
#		 raised, to simulate that exception cost to much than the
#		 flag is worth in practical uses.
#
#	Step 7.	Return 1 + X.
#		7.1	ans := X
#		7.2	Restore user FPCR.
#		7.3	Return ans := 1 + ans. Exit
#	Notes:	For non-zero X, the inexact exception will always be
#		 raised by 7.3. That is the only exception raised by 7.3.
#		 Note also that we use the FMOVEM instruction to move X
#		 in Step 7.1 to avoid unnecessary trapping. (Although
#		 the FMOVEM may not seem relevant since X is normalized,
#		 the precaution will be useful in the library version of
#		 this code where the separate entry for denormalized inputs
#		 will be done away with.)
#
#	Step 8.	Handle exp(X) where |X| >= 16380log2.
#		8.1	If |X| > 16480 log2, go to Step 9.
#		(mimic 2.2 - 2.6)
#		8.2	N := round-to-integer( X * 64/log2 )
#		8.3	Calculate J = N mod 64, J = 0,1,...,63
#		8.4	K := (N-J)/64, M1 := truncate(K/2), M = K-M1, AdjFlag := 1.
#		8.5	Calculate the address of the stored value 2^(J/64).
#		8.6	Create the values Scale = 2^M, AdjScale = 2^M1.
#		8.7	Go to Step 3.
#	Notes:	Refer to notes for 2.2 - 2.6.
#
#	Step 9.	Handle exp(X), |X| > 16480 log2.
#		9.1	If X < 0, go to 9.3
#		9.2	ans := Huge, go to 9.4
#		9.3	ans := Tiny.
#		9.4	Restore user FPCR.
#		9.5	Return ans := ans * ans. Exit.
#	Notes:	Exp(X) will surely overflow or underflow, depending on
#		 X's sign. "Huge" and "Tiny" are respectively large/tiny
#		 extended-precision numbers whose square over/underflow
#		 with an inexact result. Thus, 9.5 always raises the
#		 inexact together with either overflow or underflow.
#
#
#	setoxm1d
#	--------
#
#	Step 1.	Set ans := 0
#
#	Step 2.	Return	ans := X + ans. Exit.
#	Notes:	This will return X with the appropriate rounding
#		 precision prescribed by the user FPCR.
#
#	setoxm1
#	-------
#
#	Step 1.	Check |X|
#		1.1	If |X| >= 1/4, go to Step 1.3.
#		1.2	Go to Step 7.
#		1.3	If |X| < 70 log(2), go to Step 2.
#		1.4	Go to Step 10.
#	Notes:	The usual case should take the branches 1.1 -> 1.3 -> 2.
#		 However, it is conceivable |X| can be small very often
#		 because EXPM1 is intended to evaluate exp(X)-1 accurately
#		 when |X| is small. For further details on the comparisons,
#		 see the notes on Step 1 of setox.
#
#	Step 2.	Calculate N = round-to-nearest-int( X * 64/log2 ).
#		2.1	N := round-to-nearest-integer( X * 64/log2 ).
#		2.2	Calculate	J = N mod 64; so J = 0,1,2,..., or 63.
#		2.3	Calculate	M = (N - J)/64; so N = 64M + J.
#		2.4	Calculate the address of the stored value of 2^(J/64).
#		2.5	Create the values Sc = 2^M and OnebySc := -2^(-M).
#	Notes:	See the notes on Step 2 of setox.
#
#	Step 3.	Calculate X - N*log2/64.
#		3.1	R := X + N*L1, where L1 := single-precision(-log2/64).
#		3.2	R := R + N*L2, L2 := extended-precision(-log2/64 - L1).
#	Notes:	Applying the analysis of Step 3 of setox in this case
#		 shows that |R| <= 0.0055 (note that |X| <= 70 log2 in
#		 this case).
#
#	Step 4.	Approximate exp(R)-1 by a polynomial
#			p = R+R*R*(A1+R*(A2+R*(A3+R*(A4+R*(A5+R*A6)))))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: A1 (which is 1/2), A5 and A6
#		 are single precision; A2, A3 and A4 are double precision.
#		 b) Even with the restriction above,
#			|p - (exp(R)-1)| <	|R| * 2^(-72.7)
#		 for all |R| <= 0.0055.
#		 c) To fully utilize the pipeline, p is separated into
#		 two independent pieces of roughly equal complexity
#			p = [ R*S*(A2 + S*(A4 + S*A6)) ]	+
#				[ R + S*(A1 + S*(A3 + S*A5)) ]
#		 where S = R*R.
#
#	Step 5.	Compute 2^(J/64)*p by
#				p := T*p
#		 where T and t are the stored values for 2^(J/64).
#	Notes:	2^(J/64) is stored as T and t where T+t approximates
#		 2^(J/64) to roughly 85 bits; T is in extended precision
#		 and t is in single precision. Note also that T is rounded
#		 to 62 bits so that the last two bits of T are zero. The
#		 reason for such a special form is that T-1, T-2, and T-8
#		 will all be exact --- a property that will be exploited
#		 in Step 6 below. The total relative error in p is no
#		 bigger than 2^(-67.7) compared to the final result.
#
#	Step 6.	Reconstruction of exp(X)-1
#			exp(X)-1 = 2^M * ( 2^(J/64) + p - 2^(-M) ).
#		6.1	If M <= 63, go to Step 6.3.
#		6.2	ans := T + (p + (t + OnebySc)). Go to 6.6
#		6.3	If M >= -3, go to 6.5.
#		6.4	ans := (T + (p + t)) + OnebySc. Go to 6.6
#		6.5	ans := (T + OnebySc) + (p + t).
#		6.6	Restore user FPCR.
#		6.7	Return ans := Sc * ans. Exit.
#	Notes:	The various arrangements of the expressions give accurate
#		 evaluations.
#
#	Step 7.	exp(X)-1 for |X| < 1/4.
#		7.1	If |X| >= 2^(-65), go to Step 9.
#		7.2	Go to Step 8.
#
#	Step 8.	Calculate exp(X)-1, |X| < 2^(-65).
#		8.1	If |X| < 2^(-16312), goto 8.3
#		8.2	Restore FPCR; return ans := X - 2^(-16382). Exit.
#		8.3	X := X * 2^(140).
#		8.4	Restore FPCR; ans := ans - 2^(-16382).
#		 Return ans := ans*2^(140). Exit
#	Notes:	The idea is to return "X - tiny" under the user
#		 precision and rounding modes. To avoid unnecessary
#		 inefficiency, we stay away from denormalized numbers the
#		 best we can. For |X| >= 2^(-16312), the straightforward
#		 8.2 generates the inexact exception as the case warrants.
#
#	Step 9.	Calculate exp(X)-1, |X| < 1/4, by a polynomial
#			p = X + X*X*(B1 + X*(B2 + ... + X*B12))
#	Notes:	a) In order to reduce memory access, the coefficients are
#		 made as "short" as possible: B1 (which is 1/2), B9 to B12
#		 are single precision; B3 to B8 are double precision; and
#		 B2 is double extended.
#		 b) Even with the restriction above,
#			|p - (exp(X)-1)| < |X| 2^(-70.6)
#		 for all |X| <= 0.251.
#		 Note that 0.251 is slightly bigger than 1/4.
#		 c) To fully preserve accuracy, the polynomial is computed
#		 as	X + ( S*B1 +	Q ) where S = X*X and
#			Q	=	X*S*(B2 + X*(B3 + ... + X*B12))
#		 d) To fully utilize the pipeline, Q is separated into
#		 two independent pieces of roughly equal complexity
#			Q = [ X*S*(B2 + S*(B4 + ... + S*B12)) ] +
#				[ S*S*(B3 + S*(B5 + ... + S*B11)) ]
#
#	Step 10.	Calculate exp(X)-1 for |X| >= 70 log 2.
#		10.1 If X >= 70log2 , exp(X) - 1 = exp(X) for all practical
#		 purposes. Therefore, go to Step 1 of setox.
#		10.2 If X <= -70log2, exp(X) - 1 = -1 for all practical purposes.
#		 ans := -1
#		 Restore user FPCR
#		 Return ans := ans + 2^(-126). Exit.
#	Notes:	10.2 will always create an inexact and return -1 + tiny
#		 in the user rounding precision and mode.
#
#
                                    
#		Copyright (C) Motorola, Inc. 1990
#			All Rights Reserved
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA 
#	The copyright notice above does not evidence any  
#	actual or intended publication of such source code.
                                    
                                    
                                    
         text                       
                                    
	include(fpsp.h.s)
                                    
l2:      long      0x3fdc0000,0x82e30865,0x4361c4c6,0x00000000 
                                    
expa3:   long      0x3fa55555,0x55554431 
expa2:   long      0x3fc55555,0x55554018 
                                    
huge:    long      0x7ffe0000,0xffffffff,0xffffffff,0x00000000 
tiny:    long      0x00010000,0xffffffff,0xffffffff,0x00000000 
                                    
em1a4:   long      0x3f811111,0x11174385 
em1a3:   long      0x3fa55555,0x55554f5a 
                                    
em1a2:   long      0x3fc55555,0x55555555,0x00000000,0x00000000 
                                    
em1b8:   long      0x3ec71de3,0xa5774682 
em1b7:   long      0x3efa01a0,0x19d7cb68 
                                    
em1b6:   long      0x3f2a01a0,0x1a019df3 
em1b5:   long      0x3f56c16c,0x16c170e2 
                                    
em1b4:   long      0x3f811111,0x11111111 
em1b3:   long      0x3fa55555,0x55555555 
                                    
em1b2:   long      0x3ffc0000,0xaaaaaaaa,0xaaaaaaab 
         long      0x00000000       
                                    
two140:  long      0x48b00000,0x00000000 
twon140: long      0x37300000,0x00000000 
                                    
exptbl:                             
         long      0x3fff0000,0x80000000,0x00000000,0x00000000 
         long      0x3fff0000,0x8164d1f3,0xbc030774,0x9f841a9b 
         long      0x3fff0000,0x82cd8698,0xac2ba1d8,0x9fc1d5b9 
         long      0x3fff0000,0x843a28c3,0xacde4048,0xa0728369 
         long      0x3fff0000,0x85aac367,0xcc487b14,0x1fc5c95c 
         long      0x3fff0000,0x871f6196,0x9e8d1010,0x1ee85c9f 
         long      0x3fff0000,0x88980e80,0x92da8528,0x9fa20729 
         long      0x3fff0000,0x8a14d575,0x496efd9c,0xa07bf9af 
         long      0x3fff0000,0x8b95c1e3,0xea8bd6e8,0xa0020dcf 
         long      0x3fff0000,0x8d1adf5b,0x7e5ba9e4,0x205a63da 
         long      0x3fff0000,0x8ea4398b,0x45cd53c0,0x1eb70051 
         long      0x3fff0000,0x9031dc43,0x1466b1dc,0x1f6eb029 
         long      0x3fff0000,0x91c3d373,0xab11c338,0xa0781494 
         long      0x3fff0000,0x935a2b2f,0x13e6e92c,0x9eb319b0 
         long      0x3fff0000,0x94f4efa8,0xfef70960,0x2017457d 
         long      0x3fff0000,0x96942d37,0x20185a00,0x1f11d537 
         long      0x3fff0000,0x9837f051,0x8db8a970,0x9fb952dd 
         long      0x3fff0000,0x99e04593,0x20b7fa64,0x1fe43087 
         long      0x3fff0000,0x9b8d39b9,0xd54e5538,0x1fa2a818 
         long      0x3fff0000,0x9d3ed9a7,0x2cffb750,0x1fde494d 
         long      0x3fff0000,0x9ef53260,0x91a111ac,0x20504890 
         long      0x3fff0000,0xa0b0510f,0xb9714fc4,0xa073691c 
         long      0x3fff0000,0xa2704303,0x0c496818,0x1f9b7a05 
         long      0x3fff0000,0xa43515ae,0x09e680a0,0xa0797126 
         long      0x3fff0000,0xa5fed6a9,0xb15138ec,0xa071a140 
         long      0x3fff0000,0xa7cd93b4,0xe9653568,0x204f62da 
         long      0x3fff0000,0xa9a15ab4,0xea7c0ef8,0x1f283c4a 
         long      0x3fff0000,0xab7a39b5,0xa93ed338,0x9f9a7fdc 
         long      0x3fff0000,0xad583eea,0x42a14ac8,0xa05b3fac 
         long      0x3fff0000,0xaf3b78ad,0x690a4374,0x1fdf2610 
         long      0x3fff0000,0xb123f581,0xd2ac2590,0x9f705f90 
         long      0x3fff0000,0xb311c412,0xa9112488,0x201f678a 
         long      0x3fff0000,0xb504f333,0xf9de6484,0x1f32fb13 
         long      0x3fff0000,0xb6fd91e3,0x28d17790,0x20038b30 
         long      0x3fff0000,0xb8fbaf47,0x62fb9ee8,0x200dc3cc 
         long      0x3fff0000,0xbaff5ab2,0x133e45fc,0x9f8b2ae6 
         long      0x3fff0000,0xbd08a39f,0x580c36c0,0xa02bbf70 
         long      0x3fff0000,0xbf1799b6,0x7a731084,0xa00bf518 
         long      0x3fff0000,0xc12c4cca,0x66709458,0xa041dd41 
         long      0x3fff0000,0xc346ccda,0x24976408,0x9fdf137b 
         long      0x3fff0000,0xc5672a11,0x5506dadc,0x201f1568 
         long      0x3fff0000,0xc78d74c8,0xabb9b15c,0x1fc13a2e 
         long      0x3fff0000,0xc9b9bd86,0x6e2f27a4,0xa03f8f03 
         long      0x3fff0000,0xcbec14fe,0xf2727c5c,0x1ff4907d 
         long      0x3fff0000,0xce248c15,0x1f8480e4,0x9e6e53e4 
         long      0x3fff0000,0xd06333da,0xef2b2594,0x1fd6d45c 
         long      0x3fff0000,0xd2a81d91,0xf12ae45c,0xa076edb9 
         long      0x3fff0000,0xd4f35aab,0xcfedfa20,0x9fa6de21 
         long      0x3fff0000,0xd744fcca,0xd69d6af4,0x1ee69a2f 
         long      0x3fff0000,0xd99d15c2,0x78afd7b4,0x207f439f 
         long      0x3fff0000,0xdbfbb797,0xdaf23754,0x201ec207 
         long      0x3fff0000,0xde60f482,0x5e0e9124,0x9e8be175 
         long      0x3fff0000,0xe0ccdeec,0x2a94e110,0x20032c4b 
         long      0x3fff0000,0xe33f8972,0xbe8a5a50,0x2004dff5 
         long      0x3fff0000,0xe5b906e7,0x7c8348a8,0x1e72f47a 
         long      0x3fff0000,0xe8396a50,0x3c4bdc68,0x1f722f22 
         long      0x3fff0000,0xeac0c6e7,0xdd243930,0xa017e945 
         long      0x3fff0000,0xed4f301e,0xd9942b84,0x1f401a5b 
         long      0x3fff0000,0xefe4b99b,0xdcdaf5cc,0x9fb9a9e3 
         long      0x3fff0000,0xf281773c,0x59ffb138,0x20744c05 
         long      0x3fff0000,0xf5257d15,0x2486cc2c,0x1f773a19 
         long      0x3fff0000,0xf7d0df73,0x0ad13bb8,0x1ffe90d5 
         long      0x3fff0000,0xfa83b2db,0x722a033c,0xa041ed22 
         long      0x3fff0000,0xfd3e0c0c,0xf486c174,0x1f853f3a 
                                    
         set       adjflag,l_scr2   
         set       scale,fp_scr1    
         set       adjscale,fp_scr2 
         set       sc,fp_scr3       
         set       onebysc,fp_scr4  
                                    
                                    
                                    
                                    
                                    
                                    
         global    setoxd           
setoxd:                             
#--entry point for EXP(X), X is denormalized
         move.l    (%a0),%d0        
         andi.l    &0x80000000,%d0  
         ori.l     &0x00800000,%d0  # sign(X)*2^(-126)
         move.l    %d0,-(%sp)       
         fmove.s   &0f1.0,%fp0      
         fmove.l   %d1,%fpcr        
         fadd.s    (%sp)+,%fp0      
         bra.l     t_frcinx         
                                    
         global    setox            
setox:                              
#--entry point for EXP(X), here X is finite, non-zero, and not NaN's
                                    
#--Step 1.
         move.l    (%a0),%d0        # load part of input X
         andi.l    &0x7fff0000,%d0  # biased expo. of X
         cmpi.l    %d0,&0x3fbe0000  # 2^(-65)
         bge.b     expc1            # normal case
         bra.w     expsm            
                                    
expc1:                              
#--The case |X| >= 2^(-65)
         move.w    4(%a0),%d0       # expo. and partial sig. of |X|
         cmpi.l    %d0,&0x400cb167  # 16380 log2 trunc. 16 bits
         blt.b     expmain          # normal case
         bra.w     expbig           
                                    
expmain:                            
#--Step 2.
#--This is the normal branch:	2^(-65) <= |X| < 16380 log2.
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         move.l    &0,adjflag(%a6)  
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
         lea       exptbl,%a1       
         fmove.l   %d0,%fp0         # convert to floating-format
                                    
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M)
         move.w    l2,l_scr1(%a6)   # prefetch L2, no need in CB
                                    
expcont1:                            
#--Step 3.
#--fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
#--a0 points to 2^(J/64), D0 is biased expo. of 2^(M)
         fmove.x   %fp0,%fp2        
         fmul.s    &0xbc317218,%fp0 # N * L1, L1 = lead(-log2/64)
         fmul.x    l2,%fp2          # N * L2, L1+L2 = -log2/64
         fadd.x    %fp1,%fp0        # X + N*L1
         fadd.x    %fp2,%fp0        # fp0 is R, reduced arg.
#	MOVE.W		#$3FA5,EXPA3	...load EXPA3 in cache
                                    
#--Step 4.
#--WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
#-- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*A5))))
#--TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
#--[R+R*S*(A2+S*A4)] + [S*(A1+S*(A3+S*A5))]
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # fp1 IS S = R*R
                                    
         fmove.s   &0x3ab60b70,%fp2 # fp2 IS A5
#	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*A5
         fmove.x   %fp1,%fp3        
         fmul.s    &0x3c088895,%fp3 # fp3 IS S*A4
                                    
         fadd.d    expa3,%fp2       # fp2 IS A3+S*A5
         fadd.d    expa2,%fp3       # fp3 IS A2+S*A4
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A3+S*A5)
         move.w    %d0,scale(%a6)   # SCALE is 2^(M) in extended
         clr.w     scale+2(%a6)     
         move.l    &0x80000000,scale+4(%a6) 
         clr.l     scale+8(%a6)     
                                    
         fmul.x    %fp1,%fp3        # fp3 IS S*(A2+S*A4)
                                    
         fadd.s    &0f0.5,%fp2      # fp2 IS A1+S*(A3+S*A5)
         fmul.x    %fp0,%fp3        # fp3 IS R*S*(A2+S*A4)
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A1+S*(A3+S*A5))
         fadd.x    %fp3,%fp0        # fp0 IS R+R*S*(A2+S*A4),
#					...fp3 released
                                    
         fmove.x   (%a1)+,%fp1      # fp1 is lead. pt. of 2^(J/64)
         fadd.x    %fp2,%fp0        # fp0 is EXP(R) - 1
#					...fp2 released
                                    
#--Step 5
#--final reconstruction process
#--EXP(X) = 2^M * ( 2^(J/64) + 2^(J/64)*(EXP(R)-1) )
                                    
         fmul.x    %fp1,%fp0        # 2^(J/64)*(Exp(R)-1)
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
         fadd.s    (%a1),%fp0       # accurate 2^(J/64)
                                    
         fadd.x    %fp1,%fp0        
         move.l    adjflag(%a6),%d0 
                                    
#--Step 6
         tst.l     %d0              
         beq.b     normal           
adjust:                             
         fmul.x    adjscale(%a6),%fp0 
normal:                             
         fmove.l   %d1,%fpcr        # restore user FPCR
         fmul.x    scale(%a6),%fp0  # multiply 2^(M)
         bra.l     t_frcinx         
                                    
expsm:                              
#--Step 7
         fmovem.x  (%a0),%fp0       # in case X is denormalized
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.0,%fp0      # 1+X in user mode
         bra.l     t_frcinx         
                                    
expbig:                             
#--Step 8
         cmpi.l    %d0,&0x400cb27c  # 16480 log2
         bgt.b     exp2big          
#--Steps 8.2 -- 8.6
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         move.l    &1,adjflag(%a6)  
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
         lea       exptbl,%a1       
         fmove.l   %d0,%fp0         # convert to floating-format
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is K
         move.l    %d0,l_scr1(%a6)  # save K temporarily
         asr.l     &1,%d0           # D0 is M1
         sub.l     %d0,l_scr1(%a6)  # a1 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M1)
         move.w    %d0,adjscale(%a6) # ADJSCALE := 2^(M1)
         clr.w     adjscale+2(%a6)  
         move.l    &0x80000000,adjscale+4(%a6) 
         clr.l     adjscale+8(%a6)  
         move.l    l_scr1(%a6),%d0  # D0 is M
         addi.w    &0x3fff,%d0      # biased expo. of 2^(M)
         bra.w     expcont1         # go back to Step 3
                                    
exp2big:                            
#--Step 9
         fmove.l   %d1,%fpcr        
         move.l    (%a0),%d0        
         bclr.b    &sign_bit,(%a0)  # setox always returns positive
         cmpi.l    %d0,&0           
         blt.l     t_unfl           
         bra.l     t_ovfl           
                                    
         global    setoxm1d         
setoxm1d:                            
#--entry point for EXPM1(X), here X is denormalized
#--Step 0.
         bra.l     t_extdnrm        
                                    
                                    
         global    setoxm1          
setoxm1:                            
#--entry point for EXPM1(X), here X is finite, non-zero, non-NaN
                                    
#--Step 1.
#--Step 1.1
         move.l    (%a0),%d0        # load part of input X
         andi.l    &0x7fff0000,%d0  # biased expo. of X
         cmpi.l    %d0,&0x3ffd0000  # 1/4
         bge.b     em1con1          # |X| >= 1/4
         bra.w     em1sm            
                                    
em1con1:                            
#--Step 1.3
#--The case |X| >= 1/4
         move.w    4(%a0),%d0       # expo. and partial sig. of |X|
         cmpi.l    %d0,&0x4004c215  # 70log2 rounded up to 16 bits
         ble.b     em1main          # 1/4 <= |X| <= 70log2
         bra.w     em1big           
                                    
em1main:                            
#--Step 2.
#--This is the case:	1/4 <= |X| <= 70 log2.
         fmove.x   (%a0),%fp0       # load input from (a0)
                                    
         fmove.x   %fp0,%fp1        
         fmul.s    &0x42b8aa3b,%fp0 # 64/log2 * X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
#	MOVE.W		#$3F81,EM1A4		...prefetch in CB mode
         fmove.l   %fp0,%d0         # N = int( X * 64/log2 )
         lea       exptbl,%a1       
         fmove.l   %d0,%fp0         # convert to floating-format
                                    
         move.l    %d0,l_scr1(%a6)  # save N temporarily
         andi.l    &0x3f,%d0        # D0 is J = N mod 64
         lsl.l     &4,%d0           
         adda.l    %d0,%a1          # address of 2^(J/64)
         move.l    l_scr1(%a6),%d0  
         asr.l     &6,%d0           # D0 is M
         move.l    %d0,l_scr1(%a6)  # save a copy of M
#	MOVE.W		#$3FDC,L2		...prefetch L2 in CB mode
                                    
#--Step 3.
#--fp1,fp2 saved on the stack. fp0 is N, fp1 is X,
#--a0 points to 2^(J/64), D0 and a1 both contain M
         fmove.x   %fp0,%fp2        
         fmul.s    &0xbc317218,%fp0 # N * L1, L1 = lead(-log2/64)
         fmul.x    l2,%fp2          # N * L2, L1+L2 = -log2/64
         fadd.x    %fp1,%fp0        # X + N*L1
         fadd.x    %fp2,%fp0        # fp0 is R, reduced arg.
#	MOVE.W		#$3FC5,EM1A2		...load EM1A2 in cache
         addi.w    &0x3fff,%d0      # D0 is biased expo. of 2^M
                                    
#--Step 4.
#--WE NOW COMPUTE EXP(R)-1 BY A POLYNOMIAL
#-- R + R*R*(A1 + R*(A2 + R*(A3 + R*(A4 + R*(A5 + R*A6)))))
#--TO FULLY UTILIZE THE PIPELINE, WE COMPUTE S = R*R
#--[R*S*(A2+S*(A4+S*A6))] + [R+S*(A1+S*(A3+S*A5))]
                                    
         fmove.x   %fp0,%fp1        
         fmul.x    %fp1,%fp1        # fp1 IS S = R*R
                                    
         fmove.s   &0x3950097b,%fp2 # fp2 IS a6
#	MOVE.W		#0,2(a1)	...load 2^(J/64) in cache
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*A6
         fmove.x   %fp1,%fp3        
         fmul.s    &0x3ab60b6a,%fp3 # fp3 IS S*A5
                                    
         fadd.d    em1a4,%fp2       # fp2 IS A4+S*A6
         fadd.d    em1a3,%fp3       # fp3 IS A3+S*A5
         move.w    %d0,sc(%a6)      # SC is 2^(M) in extended
         clr.w     sc+2(%a6)        
         move.l    &0x80000000,sc+4(%a6) 
         clr.l     sc+8(%a6)        
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A4+S*A6)
         move.l    l_scr1(%a6),%d0  # D0 is	M
         neg.w     %d0              # D0 is -M
         fmul.x    %fp1,%fp3        # fp3 IS S*(A3+S*A5)
         addi.w    &0x3fff,%d0      # biased expo. of 2^(-M)
         fadd.d    em1a2,%fp2       # fp2 IS A2+S*(A4+S*A6)
         fadd.s    &0f0.5,%fp3      # fp3 IS A1+S*(A3+S*A5)
                                    
         fmul.x    %fp1,%fp2        # fp2 IS S*(A2+S*(A4+S*A6))
         ori.w     &0x8000,%d0      # signed/expo. of -2^(-M)
         move.w    %d0,onebysc(%a6) # OnebySc is -2^(-M)
         clr.w     onebysc+2(%a6)   
         move.l    &0x80000000,onebysc+4(%a6) 
         clr.l     onebysc+8(%a6)   
         fmul.x    %fp3,%fp1        # fp1 IS S*(A1+S*(A3+S*A5))
#					...fp3 released
                                    
         fmul.x    %fp0,%fp2        # fp2 IS R*S*(A2+S*(A4+S*A6))
         fadd.x    %fp1,%fp0        # fp0 IS R+S*(A1+S*(A3+S*A5))
#					...fp1 released
                                    
         fadd.x    %fp2,%fp0        # fp0 IS EXP(R)-1
#					...fp2 released
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
                                    
#--Step 5
#--Compute 2^(J/64)*p
                                    
         fmul.x    (%a1),%fp0       # 2^(J/64)*(Exp(R)-1)
                                    
#--Step 6
#--Step 6.1
         move.l    l_scr1(%a6),%d0  # retrieve M
         cmpi.l    %d0,&63          
         ble.b     mle63            
#--Step 6.2	M >= 64
         fmove.s   12(%a1),%fp1     # fp1 is t
         fadd.x    onebysc(%a6),%fp1 # fp1 is t+OnebySc
         fadd.x    %fp1,%fp0        # p+(t+OnebySc), fp1 released
         fadd.x    (%a1),%fp0       # T+(p+(t+OnebySc))
         bra.b     em1scale         
mle63:                              
#--Step 6.3	M <= 63
         cmpi.l    %d0,&-3          
         bge.b     mgen3            
mltn3:                              
#--Step 6.4	M <= -4
         fadd.s    12(%a1),%fp0     # p+t
         fadd.x    (%a1),%fp0       # T+(p+t)
         fadd.x    onebysc(%a6),%fp0 # OnebySc + (T+(p+t))
         bra.b     em1scale         
mgen3:                              
#--Step 6.5	-3 <= M <= 63
         fmove.x   (%a1)+,%fp1      # fp1 is T
         fadd.s    (%a1),%fp0       # fp0 is p+t
         fadd.x    onebysc(%a6),%fp1 # fp1 is T+OnebySc
         fadd.x    %fp1,%fp0        # (T+OnebySc)+(p+t)
                                    
em1scale:                            
#--Step 6.6
         fmove.l   %d1,%fpcr        
         fmul.x    sc(%a6),%fp0     
                                    
         bra.l     t_frcinx         
                                    
em1sm:                              
#--Step 7	|X| < 1/4.
         cmpi.l    %d0,&0x3fbe0000  # 2^(-65)
         bge.b     em1poly          
                                    
em1tiny:                            
#--Step 8	|X| < 2^(-65)
         cmpi.l    %d0,&0x00330000  # 2^(-16312)
         blt.b     em12tiny         
#--Step 8.2
         move.l    &0x80010000,sc(%a6) # SC is -2^(-16382)
         move.l    &0x80000000,sc+4(%a6) 
         clr.l     sc+8(%a6)        
         fmove.x   (%a0),%fp0       
         fmove.l   %d1,%fpcr        
         fadd.x    sc(%a6),%fp0     
                                    
         bra.l     t_frcinx         
                                    
em12tiny:                            
#--Step 8.3
         fmove.x   (%a0),%fp0       
         fmul.d    two140,%fp0      
         move.l    &0x80010000,sc(%a6) 
         move.l    &0x80000000,sc+4(%a6) 
         clr.l     sc+8(%a6)        
         fadd.x    sc(%a6),%fp0     
         fmove.l   %d1,%fpcr        
         fmul.d    twon140,%fp0     
                                    
         bra.l     t_frcinx         
                                    
em1poly:                            
#--Step 9	exp(X)-1 by a simple polynomial
         fmove.x   (%a0),%fp0       # fp0 is X
         fmul.x    %fp0,%fp0        # fp0 is S := X*X
         fmovem.x  %fp2/%fp3,-(%a7) # save fp2
         fmove.s   &0x2f30caa8,%fp1 # fp1 is B12
         fmul.x    %fp0,%fp1        # fp1 is S*B12
         fmove.s   &0x310f8290,%fp2 # fp2 is B11
         fadd.s    &0x32d73220,%fp1 # fp1 is B10+S*B12
                                    
         fmul.x    %fp0,%fp2        # fp2 is S*B11
         fmul.x    %fp0,%fp1        
                                    
         fadd.s    &0x3493f281,%fp2 
         fadd.d    em1b8,%fp1       
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
         fadd.d    em1b7,%fp2       
         fadd.d    em1b6,%fp1       
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
         fadd.d    em1b5,%fp2       
         fadd.d    em1b4,%fp1       
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
         fadd.d    em1b3,%fp2       
         fadd.x    em1b2,%fp1       
                                    
         fmul.x    %fp0,%fp2        
         fmul.x    %fp0,%fp1        
                                    
         fmul.x    %fp0,%fp2        # )
         fmul.x    (%a0),%fp1       
                                    
         fmul.s    &0f0.5,%fp0      # fp0 is S*B1
         fadd.x    %fp2,%fp1        # fp1 is Q
#					...fp2 released
                                    
         fmovem.x  (%a7)+,%fp2/%fp3 # fp2 restored
                                    
         fadd.x    %fp1,%fp0        # fp0 is S*B1+Q
#					...fp1 released
                                    
         fmove.l   %d1,%fpcr        
         fadd.x    (%a0),%fp0       
                                    
         bra.l     t_frcinx         
                                    
em1big:                             
#--Step 10	|X| > 70 log2
         move.l    (%a0),%d0        
         cmpi.l    %d0,&0           
         bgt.w     expc1            
#--Step 10.2
         fmove.s   &0f-1.0,%fp0     # fp0 is -1
         fmove.l   %d1,%fpcr        
         fadd.s    &0f1.1754943e-38,%fp0 # -1 + 2^(-126)
                                    
         bra.l     t_frcinx         
                                    
                                    
	version 3
