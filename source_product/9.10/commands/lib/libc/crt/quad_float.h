/*
 * @(#)quad_float.h: $Revision: 64.1 $ $Date: 89/02/09 11:22:13 $
 * $Locker:  $
 * 
 */
/**************************************
 * Declare quad precision functions *
 **************************************/

/* 32-bit word grabing functions */
#define Quad_firstword(value) Qallp1(value)
#define Quad_secondword(value) Qallp2(value)
#define Quad_thirdword(value) Qallp3(value)
#define Quad_fourthword(value) Qallp4(value)

#define Quad_sign(object) Qsign(object)
#define Quad_exponent(object) Qexponent(object)
#define Quad_signexponent(object) Qsignexponent(object)
#define Quad_mantissap1(object) Qmantissap1(object)
#define Quad_mantissap2(object) Qmantissap2(object)
#define Quad_mantissap3(object) Qmantissap3(object)
#define Quad_mantissap4(object) Qmantissap4(object)
#define Quad_exponentmantissap1(object) Qexponentmantissap1(object)
#define Quad_allp1(object) Qallp1(object)
#define Quad_allp2(object) Qallp2(object)
#define Quad_allp3(object) Qallp3(object)
#define Quad_allp4(object) Qallp4(object)

/* quad_and_signs ands the sign bits of each argument and puts the result
 * into the first argument. quad_or_signs ors those same sign bits */
#define Quad_and_signs( src1dst, src2)		\
    Qallp1(src1dst) = (Qallp1(src2)|~(1<<31)) & Qallp1(src1dst)
#define Quad_or_signs( src1dst, src2)		\
    Qallp1(src1dst) = (Qallp1(src2)&(1<<31)) | Qallp1(src1dst)

/* This magnitude comparison uses the signless first words and
 * the regular part2 words.  The comparison is graphically:
 *
 *       1st greater?  ----------->|
 *                                 |
 *       1st less?-----------------+------->|
 *                                 |        |
 *       2nd greater?------------->|        |
 *                                 |        |
 *       2nd less?-----------------+------->|
 *                                 |        |
 *       3rd greater?------------->|        |
 *                                 |        |
 *       3rd less?-----------------+------->|
 *                                 |        |
 *       4th greater or equal?---->|        |
 *                                 |        |
 *                               False     True
 */
#define Quad_ismagnitudeless(leftp2,leftp3,leftp4,rightp2,rightp3,rightp4,signlessleft,signlessright) \
/*  Quad_floating_point left, right;          *				\
 *  unsigned int signlessleft, signlessright; */			\
      ( signlessleft<=signlessright &&					\
       (signlessleft<signlessright || (Qallp2(leftp2)<=Qallp2(rightp2) && \
        (Qallp2(leftp2)<Qallp2(rightp2) || (Qallp3(leftp3)<=Qallp3(rightp3) && \
	 (Qallp3(leftp3)<Qallp3(rightp3) || Qallp4(leftp4)<Qallp4(rightp4)))))))
         
#define Quad_xortointp1(leftp1,rightp1,result)			\
    /* quad_floating_point left, right;				\
     * unsigned int result; */					\
    result = Qallp1(leftp1) XOR Qallp1(rightp1)

#define Quad_xorfromintp1(leftp1,rightp1,resultp1)		\
    /* quad_floating_point right, result;			\
     * unsigned int left; */					\
    Qallp1(resultp1) = leftp1 XOR Qallp1(rightp1)

#define Quad_swap_lower(leftp2,leftp3,leftp4,rightp2,rightp3,rightp4)  \
    /* quad_floating_point left, right; */			\
    Qallp2(leftp2)  = Qallp2(leftp2) XOR Qallp2(rightp2);	\
    Qallp2(rightp2) = Qallp2(leftp2) XOR Qallp2(rightp2);	\
    Qallp2(leftp2)  = Qallp2(leftp2) XOR Qallp2(rightp2);	\
    Qallp3(leftp3)  = Qallp3(leftp3) XOR Qallp3(rightp3);	\
    Qallp3(rightp3) = Qallp3(leftp3) XOR Qallp3(rightp3);	\
    Qallp3(leftp3)  = Qallp3(leftp3) XOR Qallp3(rightp3);	\
    Qallp4(leftp4)  = Qallp4(leftp4) XOR Qallp4(rightp4);	\
    Qallp4(rightp4) = Qallp4(leftp4) XOR Qallp4(rightp4);	\
    Qallp4(leftp4)  = Qallp4(leftp4) XOR Qallp4(rightp4)

/* The hidden bit is always the low bit of the exponent */
#define Quad_clear_exponent_set_hidden(srcdst) Deposit_qexponent(srcdst,1)
#define Quad_clear_signexponent_set_hidden(srcdst) \
    Deposit_qsignexponent(srcdst,1)
#define Quad_clear_sign(srcdst) Qallp1(srcdst) &= ~(1<<31)
#define Quad_clear_signexponent(srcdst) \
    Qallp1(srcdst) &= Qmantissap1((unsigned)-1)

/* Exponent field for quads has already been cleared and may be
 * included in the shift.  Here we need to generate two quad width
 * variable shifts.  The insignificant bits can be ignored.
 *      MTSAR f(varamount)
 *      VSHD	srcdst3,srcdst4 => srcdst4
 *      VSHD	srcdst2,srcdst3 => srcdst3
 *      VSHD	srcdst1,srcdst2 => srcdst2
 *	VSHD	0,srcdst1 => srcdst1 
 * This is very difficult to model with C expressions since the shift amount
 * could exceed 32.  */
/* varamount must be less than 128 */
#define Quad_rightshift(srcdstA, srcdstB, srcdstC, srcdstD, varamount)	\
    {int shiftamt;							\
	shiftamt = (varamount) % 32;					\
	switch((varamount)/32) {					\
	case 0:	Variableshiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp4(srcdstD));				\
		Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp3(srcdstC));				\
		Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp2(srcdstB));				\
		Qallp1(srcdstA) >>= shiftamt;				\
		break;							\
	case 1:	Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp4(srcdstD));				\
		Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp3(srcdstC));				\
		Qallp2(srcdstB) = Qallp1(srcdstA) >> shiftamt;		\
		Qallp1(srcdstA) = 0;					\
		break;							\
	case 2: Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp4(srcdstD));				\
		Qallp3(srcdstC) = Qallp1(srcdstA) >> shiftamt;		\
		Qallp1(srcdstA) = Qallp2(srcdstB) = 0;			\
		break;							\
	case 3: Qallp4(srcdstD) = Qallp1(srcdstA) >> shiftamt;		\
		Qallp1(srcdstA) = Qallp2(srcdstB) = Qallp3(srcdstC) = 0; \
		break;							\
	}								\
    }

/* varamount must be less than 128 */
#define Quad_rightshift_exponentmantissa(srcdstA,srcdstB,srcdstC,srcdstD,varamount) \
    {int shiftamt;							\
	shiftamt = (varamount) % 32;					\
	switch((varamount)/32) {					\
	case 0:	Variableshiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp4(srcdstD));				\
		Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp3(srcdstC));				\
		Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp2(srcdstB));				\
		Deposit_qexponentmantissap1(srcdstA,			\
		    (Qexponentmantissap1(srcdstA)>>shiftamt));		\
		break;							\
	case 1:	Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp4(srcdstD));				\
		Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp3(srcdstC));				\
		Qallp2(srcdstB) = Qexponentmantissap1(srcdstA) >> shiftamt; \
		Deposit_qexponentmantissap1(srcdstA,0);			\
		break;							\
	case 2: Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp4(srcdstD));				\
		Qallp3(srcdstC) = Qexponentmantissap1(srcdstA) >> shiftamt; \
		Qallp2(srcdstB) = 0;					\
		Deposit_qexponentmantissap1(srcdstA,0);			\
		break;							\
	case 3: Qallp4(srcdstD) = Qexponentmantissap1(srcdstA) >> shiftamt; \
		Qallp3(srcdstC) = Qallp2(srcdstB) = 0; 			\
		Deposit_qexponentmantissap1(srcdstA,0);			\
		break;							\
	}								\
    }

/* varamount must be less than 128 */
#define Quad_leftshift(srcdstA, srcdstB, srcdstC, srcdstD, varamount)	\
    {int shiftamt;							\
	shiftamt = 31 - ((varamount+31) % 32);				\
	switch((varamount+31)/32) {					\
	case 0:	break;							\
	case 1:	Variableshiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp1(srcdstA));				\
		Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp2(srcdstB));				\
		Variableshiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp3(srcdstC));				\
		Variableshiftdouble(Qallp4(srcdstD),0,shiftamt,		\
		 Qallp4(srcdstD));					\
		break;							\
	case 2:	Variableshiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp1(srcdstA));				\
		Variableshiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp2(srcdstB));				\
		Variableshiftdouble(Qallp4(srcdstD),0,shiftamt,		\
		 Qallp3(srcdstC));					\
		Qallp4(srcdstD) = 0;					\
		break;							\
	case 3: Variableshiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp1(srcdstA));				\
		Variableshiftdouble(Qallp4(srcdstD),0,shiftamt,		\
		 Qallp2(srcdstB));					\
		Qallp3(srcdstC) = Qallp4(srcdstD) = 0;			\
		break;							\
	case 4: Variableshiftdouble(Qallp4(srcdstD),0,shiftamt,		\
		 Qallp1(srcdstA));					\
		Qallp2(srcdstB) = Qallp3(srcdstC) = Qallp4(srcdstD) = 0; \
		break;							\
	}								\
    }

#define Quad_leftshiftby1_withextent(lefta,leftb,leftc,leftd,right,resulta,resultb,resultc,resultd) \
    Shiftdouble(Qallp1(lefta), Qallp2(leftb), 31, Qallp1(resulta));	\
    Shiftdouble(Qallp2(leftb), Qallp3(leftc), 31, Qallp2(resultb));	\
    Shiftdouble(Qallp3(leftc), Qallp4(leftd), 31, Qallp3(resultc));	\
    Shiftdouble(Qallp4(leftd), Extall(right), 31, Qallp4(resultd)) 
    
#define Quad_rightshiftby1_withextent(leftd,right,dst)		\
    Extall(dst) = (Qallp4(leftd) << 31) | ((unsigned)Extall(right) >> 1) | \
		  Extlow(right)

#define Quad_arithrightshiftby1(srcdstA,srcdstB,srcdstC,srcdstD)	\
    Shiftdouble(Qallp3(srcdstC),Qallp4(srcdstD),1,Qallp4(srcdstD));	\
    Shiftdouble(Qallp2(srcdstB),Qallp3(srcdstC),1,Qallp3(srcdstC));	\
    Shiftdouble(Qallp1(srcdstA),Qallp2(srcdstB),1,Qallp2(srcdstB));	\
    Qallp1(srcdstA) = (int)Qallp1(srcdstA) >> 1
   
/* Sign extend the sign bit with an integer destination */
#define Quad_signextendedsign(value)  Qsignedsign(value)

#define Quad_isone_hidden(qval) (Is_qhidden(qval)!=0)
/* Singles and doubles may include the sign and exponent fields.  The
 * hidden bit and the hidden overflow must be included. */
#define Quad_increment(qvalA,qvalB,qvalC,qvalD) \
    if ((Qallp4(qvalD) += 1) == 0 )		\
       if ((Qallp3(qvalC) += 1) == 0 )		\
          if ((Qallp2(qvalB) += 1) == 0 )  Qallp1(qvalA) += 1
#define Quad_increment_mantissa(qvalA,qvalB,qvalC,qvalD) \
    if ((Qmantissap4(qvalD) += 1) == 0 )		 \
       if ((Qmantissap3(qvalC) += 1) == 0 )		 \
          if ((Qmantissap2(qvalB) += 1) == 0 )		 \
             Deposit_dmantissap1(qvalA,qvalA+1)

#define Quad_isone_sign(qval) (Is_qsign(qval)!=0)
#define Quad_isone_hiddenoverflow(qval) (Is_qhiddenoverflow(qval)!=0)
#define Quad_isone_lowmantissap1(qvalA) (Is_qlowp1(qvalA)!=0)
#define Quad_isone_lowmantissap2(qvalB) (Is_qlowp2(qvalB)!=0)
#define Quad_isone_lowmantissap3(qvalC) (Is_qlowp3(qvalC)!=0)
#define Quad_isone_lowmantissap4(qvalD) (Is_qlowp4(qvalD)!=0)

#ifdef __hp9000s300
#define Quad_isone_signaling(qval) (Is_qsignaling(qval)==0)
#define Quad_is_signalingnan(qval) (Qsignalingnan(qval)==0xfffe)
#else
#define Quad_isone_signaling(qval) (Is_qsignaling(qval)!=0)
#define Quad_is_signalingnan(qval) (Qsignalingnan(qval)==0xffff)
#endif

#define Quad_isnotzero(qvalA,qvalB,qvalC,qvalD) \
    (Qallp1(qvalA) || Qallp2(qvalB) || Qallp3(qvalC) || Qallp4(qvalD))
#define Quad_isnotzero_hiddenhigh7mantissa(qval) \
    (Qhiddenhigh7mantissa(qval)!=0)
#define Quad_isnotzero_exponent(qval) (Qexponent(qval)!=0)
#define Quad_isnotzero_mantissa(qvalA,qvalB,qvalC,qvalD)	\
    (Qmantissap1(qvalA) || Qmantissap2(qvalB) || 		\
     Qmantissap3(qvalC) || Qmantissap4(qvalD))
#define Quad_isnotzero_mantissap1(qvalA) (Qmantissap1(qvalA)!=0)
#define Quad_isnotzero_mantissap2(qvalB) (Qmantissap2(qvalB)!=0)
#define Quad_isnotzero_mantissap3(qvalC) (Qmantissap3(qvalC)!=0)
#define Quad_isnotzero_mantissap4(qvalD) (Qmantissap4(qvalD)!=0)
#define Quad_isnotzero_exponentmantissa(qvalA,qvalB,qvalC,qvalD) \
    (Qexponentmantissap1(qvalA) || Qmantissap2(qvalB) ||	 \
     Qmantissap3(qvalC) || Qmantissap4(qvalD))
#define Quad_isnotzero_low4p4(qval) (Qlow4p4(qval)!=0)
#define Quad_iszero(qvalA,qvalB,qvalC,qvalD) (Qallp1(qvalA)==0 && \
    Qallp2(qvalB)==0 && Qallp3(qvalC)==0 && Qallp4(qvalD)==0)
#define Quad_iszero_allp1(qval) (Qallp1(qval)==0)
#define Quad_iszero_allp2(qval) (Qallp2(qval)==0)
#define Quad_iszero_allp3(qval) (Qallp3(qval)==0)
#define Quad_iszero_allp4(qval) (Qallp4(qval)==0)
#define Quad_iszero_hidden(qval) (Is_qhidden(qval)==0)
#define Quad_iszero_hiddenhigh3mantissa(qval) \
    (Qhiddenhigh3mantissa(qval)==0)
#define Quad_iszero_hiddenhigh7mantissa(qval) \
    (Qhiddenhigh7mantissa(qval)==0)
#define Quad_iszero_sign(qval) (Is_qsign(qval)==0)
#define Quad_iszero_exponent(qval) (Qexponent(qval)==0)
#define Quad_iszero_mantissa(qvalA,qvalB,qvalC,qvalD)	\
    (Qmantissap1(qvalA)==0 && Qmantissap2(qvalB)==0 &&	\
     Qmantissap3(qvalC)==0 && Qmantissap4(qvalD)==0)
#define Quad_iszero_exponentmantissa(qvalA,qvalB,qvalC,qvalD)	\
    (Qexponentmantissap1(qvalA)==0 && Qmantissap2(qvalB)==0 &&	\
     Qmantissap3(qvalC)==0 && Qmantissap4(qvalD)==0)
#define Quad_isinfinity_exponent(qval)			\
    (Qexponent(qval)==QUAD_INFINITY_EXPONENT)
#define Quad_isnotinfinity_exponent(qval)		\
    (Qexponent(qval)!=QUAD_INFINITY_EXPONENT)
#define Quad_isinfinity(qvalA,qvalB,qvalC,qvalD)	\
    (Qexponent(qvalA)==QUAD_INFINITY_EXPONENT &&	\
     Qmantissap1(qvalA)==0 && Qmantissap2(qvalB)==0 && 	\
     Qmantissap3(qvalC)==0 && Qmantissap4(qvalD)==0)
#define Quad_isnan(qvalA,qvalB,qvalC,qvalD)		\
    (Qexponent(qvalA)==QUAD_INFINITY_EXPONENT &&	\
     (Qmantissap1(qvalA)!=0 || Qmantissap2(qvalB)!=0 ||	\
      Qmantissap3(qvalC)!=0 || Qmantissap4(qvalD)!=0))
#define Quad_isnotnan(qvalA,qvalB,qvalC,qvalD)		\
    (Qexponent(qvalA)!=QUAD_INFINITY_EXPONENT ||	\
     (Qmantissap1(qvalA)==0 && Qmantissap2(qvalB)==0 &&	\
      Qmantissap3(qvalC)==0 && Qmantissap4(qvalD)==0))

#define Quad_islessthan(qop1a,qop1b,qop1c,qop1d,qop2a,qop2b,qop2c,qop2d) \
    (Qallp1(qop1a) < Qallp1(qop2a) ||			\
     (Qallp1(qop1a) == Qallp1(qop2a) &&			\
      (Qallp2(qop1b) < Qallp2(qop2b) ||			\
       (Qallp2(qop1b) == Qallp2(qop2b) &&		\
        (Qallp3(qop1c) < Qallp3(qop2c) ||		\
         (Qallp3(qop1c) == Qallp3(qop2c) &&		\
          Qallp4(qop1d) < Qallp4(qop2d)))))))
#define Quad_isgreaterthan(qop1a,qop1b,qop1c,qop1d,qop2a,qop2b,qop2c,qop2d) \
    (Qallp1(qop1a) > Qallp1(qop2a) ||			\
     (Qallp1(qop1a) == Qallp1(qop2a) &&			\
      (Qallp2(qop1b) > Qallp2(qop2b) ||			\
       (Qallp2(qop1b) == Qallp2(qop2b) &&		\
        (Qallp3(qop1c) > Qallp3(qop2c) ||		\
         (Qallp3(qop1c) == Qallp3(qop2c) &&		\
          Qallp4(qop1d) > Qallp4(qop2d)))))))
#define Quad_isnotlessthan(qop1a,qop1b,qop1c,qop1d,qop2a,qop2b,qop2c,qop2d) \
    (Qallp1(qop1a) > Qallp1(qop2a) ||			\
     (Qallp1(qop1a) == Qallp1(qop2a) &&			\
      (Qallp2(qop1b) > Qallp2(qop2b) ||			\
       (Qallp2(qop1b) == Qallp2(qop2b) &&		\
        (Qallp3(qop1c) > Qallp3(qop2c) ||		\
         (Qallp3(qop1c) == Qallp3(qop2c) &&		\
          Qallp4(qop1d) >= Qallp4(qop2d)))))))
#define Quad_isnotgreaterthan(qop1a,qop1b,qop1c,qop1d,qop2a,qop2b,qop2c,qop2d) \
    (Qallp1(qop1a) < Qallp1(qop2a) ||			\
     (Qallp1(qop1a) == Qallp1(qop2a) &&			\
      (Qallp2(qop1b) < Qallp2(qop2b) ||			\
       (Qallp2(qop1b) == Qallp2(qop2b) &&		\
        (Qallp3(qop1c) < Qallp3(qop2c) ||		\
         (Qallp3(qop1c) == Qallp3(qop2c) &&		\
          Qallp4(qop1d) <= Qallp4(qop2d)))))))
#define Quad_isequal(qop1a,qop1b,qop1c,qop1d,qop2a,qop2b,qop2c,qop2d)	\
     ((Qallp1(qop1a) == Qallp1(qop2a)) &&			\
      (Qallp2(qop1b) == Qallp2(qop2b)) &&			\
      (Qallp3(qop1c) == Qallp3(qop2c)) &&			\
      (Qallp4(qop1d) == Qallp4(qop2d)))

#define Quad_leftshiftby8(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),24,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),24,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),24,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 8
#define Quad_leftshiftby7(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),25,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),25,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),25,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 7
#define Quad_leftshiftby4(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),28,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),28,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),28,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 4
#define Quad_leftshiftby3(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),29,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),29,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),29,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 3
#define Quad_leftshiftby2(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),30,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),30,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),30,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 2
#define Quad_leftshiftby1(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),31,Qallp1(qvalA)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),31,Qallp2(qvalB)); \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),31,Qallp3(qvalC)); \
    Qallp4(qvalD) <<= 1

#define Quad_rightshiftby8(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),8,Qallp4(qvalD)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),8,Qallp3(qvalC)); \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),8,Qallp2(qvalB)); \
    Qallp1(qvalA) >>= 8
#define Quad_rightshiftby4(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),4,Qallp4(qvalD)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),4,Qallp3(qvalC)); \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),4,Qallp2(qvalB)); \
    Qallp1(qvalA) >>= 4
#define Quad_rightshiftby2(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),2,Qallp4(qvalD)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),2,Qallp3(qvalC)); \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),2,Qallp2(qvalB)); \
    Qallp1(qvalA) >>= 2
#define Quad_rightshiftby1(qvalA,qvalB,qvalC,qvalD) \
    Shiftdouble(Qallp3(qvalC),Qallp4(qvalD),1,Qallp4(qvalD)); \
    Shiftdouble(Qallp2(qvalB),Qallp3(qvalC),1,Qallp3(qvalC)); \
    Shiftdouble(Qallp1(qvalA),Qallp2(qvalB),1,Qallp2(qvalB)); \
    Qallp1(qvalA) >>= 1
    
#define Quad_copytoint_exponentmantissap1(src,dest) \
    dest = Qexponentmantissap1(src)

/* A quiet NaN has the high mantissa bit clear and at least on other (in this
 * case the adjacent bit) bit set. */
#ifdef __hp9000s300
#define Quad_set_quiet(qval) Deposit_qhigh2mantissa(qval,2)
#else
#define Quad_set_quiet(qval) Deposit_qhigh2mantissa(qval,1)
#endif

#define Quad_set_exponent(qval, exp) Deposit_qexponent(qval,exp)

#define Quad_set_mantissa(desta,destb,destc,destd,valuea,valueb,valuec,valued) \
    Deposit_qmantissap1(desta,valuea);			\
    Qmantissap2(destb) = Qmantissap2(valueb);		\
    Qmantissap3(destc) = Qmantissap3(valuec);		\
    Qmantissap4(destd) = Qmantissap4(valued)
#define Quad_set_mantissap1(desta,valuea)		\
    Deposit_qmantissap1(desta,valuea)
#define Quad_set_mantissap2(destb,valueb)		\
    Qmantissap2(destb) = Qmantissap2(valueb)
#define Quad_set_mantissap3(destc,valuec)		\
    Qmantissap3(destc) = Qmantissap3(valuec)
#define Quad_set_mantissap4(destd,valued)		\
    Qmantissap4(destd) = Qmantissap4(valued)

#define Quad_set_exponentmantissa(desta,destb,destc,destd,valuea,valueb,valuec,valued)	\
    Deposit_qexponentmantissap1(desta,valuea);			\
    Qmantissap2(destb) = Qmantissap2(valueb);			\
    Qmantissap3(destc) = Qmantissap3(valuec);			\
    Qmantissap4(destd) = Qmantissap4(valued)
#define Quad_set_exponentmantissap1(dest,value)			\
    Deposit_qexponentmantissap1(dest,value)

#define Quad_copyfromptr(src,desta,destb,destc,destd)	\
    Qallp1(desta) = src->wd0;				\
    Qallp2(destb) = src->wd1;				\
    Qallp3(destc) = src->wd2;				\
    Qallp4(destd) = src->wd3
#define Quad_copytoptr(srca,srcb,srcc,srcd,dest)	\
    dest->wd0 = Qallp1(srca);				\
    dest->wd1 = Qallp2(srcb);				\
    dest->wd2 = Qallp3(srcc);				\
    dest->wd3 = Qallp4(srcd)

/*  An infinity is represented with the max exponent and a zero mantissa */
#define Quad_setinfinity_exponent(qval) 			\
    Deposit_qexponent(qval,QUAD_INFINITY_EXPONENT)
#define Quad_setinfinity_exponentmantissa(qvalA,qvalB,qvalC,qvalD) \
    Deposit_qexponentmantissap1(qvalA, 				\
    (QUAD_INFINITY_EXPONENT << (32-(1+QUAD_EXP_LENGTH))));	\
    Qmantissap2(qvalB) = 0;					\
    Qmantissap3(qvalC) = 0;					\
    Qmantissap4(qvalD) = 0
#define Quad_setinfinitypositive(qvalA,qvalB,qvalC,qvalD)	\
    Qallp1(qvalA) 						\
        = (QUAD_INFINITY_EXPONENT << (32-(1+QUAD_EXP_LENGTH)));	\
    Qmantissap2(qvalB) = 0;					\
    Qmantissap3(qvalC) = 0;					\
    Qmantissap4(qvalD) = 0
#define Quad_setinfinitynegative(qvalA,qvalB,qvalC,qvalD)	\
    Qallp1(qvalA) = (1<<31) |					\
         (QUAD_INFINITY_EXPONENT << (32-(1+QUAD_EXP_LENGTH)));	\
    Qmantissap2(qvalB) = 0;					\
    Qmantissap3(qvalC) = 0;					\
    Qmantissap4(qvalD) = 0
#define Quad_setinfinity(qvalA,qvalB,qvalC,qvalD,sign)		\
    Qallp1(qvalA) = (sign << 31) | 				\
	(QUAD_INFINITY_EXPONENT << (32-(1+DBL_EXP_LENGTH)));	\
    Qmantissap2(qvalB) = 0;					\
    Qmantissap3(qvalC) = 0;					\
    Qmantissap4(qvalD) = 0

#define Quad_sethigh4bits(qval, extsign) Deposit_qhigh4p1(qval,extsign)
#define Quad_set_sign(qval,sign) Deposit_qsign(qval,sign)
#define Quad_invert_sign(qval) Deposit_qsign(qval,~Qsign(qval))
#define Quad_setone_sign(qval) Deposit_qsign(qval,1)
#define Quad_setone_lowmantissap4(qval) Deposit_qlowp4(qval,1)
#define Quad_setzero_sign(qval) Qallp1(qval) &= 0x7fffffff
#define Quad_setzero_exponent(qval) 		\
    Qallp1(qval) &= 0x8000ffff
#define Quad_setzero_mantissa(qvalA,qvalB,qvalC,qvalD)	\
    Qallp1(qvalA) &= 0xffff0000; Qallp2(qvalB) = 0;	\
    Qallp3(qvalC) = 0; Qallp4(qvalD) = 0
#define Quad_setzero_mantissap1(qval) Qallp1(qval) &= 0xffff0000
#define Quad_setzero_mantissap2(qval) Qallp2(qval) = 0
#define Quad_setzero_mantissap3(qval) Qallp3(qval) = 0
#define Quad_setzero_mantissap4(qval) Qallp4(qval) = 0
#define Quad_setzero_exponentmantissa(qvalA,qvalB,qvalC,qvalD)	\
    Qallp1(qvalA) &= 0x80000000; Qallp2(qvalB) = 0;		\
    Qallp3(qvalC) = 0; Qallp4(qvalD) = 0
#define Quad_setzero_exponentmantissap1(qvalA)	\
    Qallp1(qvalA) &= 0x80000000
#define Quad_setzero(qvalA,qvalB,qvalC,qvalD) 			\
    Qallp1(qvalA) = 0; Qallp2(qvalB) = 0;			\
    Qallp3(qvalC) = 0; Qallp4(qvalD) = 0
#define Quad_setzerop1(qval) Qallp1(qval) = 0
#define Quad_setzerop2(qval) Qallp2(qval) = 0
#define Quad_setzerop3(qval) Qallp3(qval) = 0
#define Quad_setzerop4(qval) Qallp4(qval) = 0
#define Quad_setnegativezero(qvalA,qvalB,qvalC,qvalD)	\
    Qallp1(qvalA) = 1 << 31; Qallp2(qvalB) = 0;		\
    Qallp3(qvalC) = 0; Qallp4(qvalD) = 0
#define Quad_setnegativezerop1(qval) Qallp1(qval) = 1 << 31

/* Use the following macro for both overflow & underflow conditions */
#define ovfl -
#define unfl +
#define Quad_setwrapped_exponent(quad_value,exponent,op) \
    Deposit_qexponent(quad_value,(exponent op QUAD_WRAP))

#define Quad_setlargestpositive(qvalA,qvalB,qvalC,qvalD)		\
    Qallp1(qvalA) = ((QUAD_EMAX+QUAD_BIAS) << (32-(1+QUAD_EXP_LENGTH))) \
			| ((1<<(32-(1+QUAD_EXP_LENGTH))) - 1 );		\
    Qallp2(qvalB) = 0xffffffff; Qallp3(qvalC) = 0xffffffff;		\
    Qallp4(qvalD) = 0xffffffff
#define Quad_setlargestnegative(qvalA,qvalB,qvalC,qvalD)		\
    Qallp1(qvalA) = ((QUAD_EMAX+QUAD_BIAS) << (32-(1+QUAD_EXP_LENGTH))) \
			| ((1<<(32-(1+QUAD_EXP_LENGTH))) - 1 ) | (1<<31); \
    Qallp2(qvalB) = 0xffffffff; Qallp3(qvalC) = 0xffffffff;		 \
    Qallp4(qvalD) = 0xffffffff
#define Quad_setlargest_exponentmantissa(qvalA,qvalB,qvalC,qvalD)	\
    Deposit_qexponentmantissap1(qvalA,					\
	(((QUAD_EMAX+QUAD_BIAS) << (32-(1+QUAD_EXP_LENGTH)))		\
			| ((1<<(32-(1+QUAD_EXP_LENGTH))) - 1 )));	\
    Qallp2(qvalB) = 0xffffffff; Qallp3(qvalC) = 0xffffffff;		\
    Qallp4(qvalD) = 0xffffffff

#define Quad_setnegativeinfinity(qvalA,qvalB,qvalC,qvalD)		\
    Qallp1(qvalA) = ((1<<QUAD_EXP_LENGTH) | QUAD_INFINITY_EXPONENT) 	\
			 << (32-(1+QUAD_EXP_LENGTH)) ; 			\
    Qallp2(qvalB) = 0; Qallp3(qvalC) = 0; Qallp4(qvalD) = 0
#define Quad_setlargest(qvalA,qvalB,qvalC,qvalD,sign)			\
    Qallp1(qvalA) = (sign << 31) |					\
         ((QUAD_EMAX+QUAD_BIAS) << (32-(1+QUAD_EXP_LENGTH))) |	 	\
	 ((1 << (32-(1+QUAD_EXP_LENGTH))) - 1 );			\
    Qallp2(qvalB) = 0xffffffff; Qallp3(qvalC) = 0xffffffff;		\
    Qallp4(qvalD) = 0xffffffff
    

/* The high bit is always zero so arithmetic or logical shifts will work. */
#define Quad_right_align(srcdstA,srcdstB,srcdstC,srcdstD,shift,extent)	\
  {int shiftamt;							\
    shiftamt = shift % 32;						\
    switch (shift/32) {							\
     case 0: if (shiftamt > 0) {					\
	        Extall(extent) = Qallp4(srcdstD) << 32 - (shiftamt);	\
                Variable_shift_double(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Qallp4(srcdstD));				\
                Variable_shift_double(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp3(srcdstC));				\
                Variable_shift_double(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp2(srcdstB));				\
	        Qallp1(srcdstA) >>= shiftamt;				\
	     }								\
	     else Extall(extent) = 0;					\
	     break;							\
     case 1: if (shiftamt > 0) {					\
                Variable_shift_double(Qallp3(srcdstC),Qallp4(srcdstD),	\
		 shiftamt,Extall(extent));				\
		if (Qallp4(srcdstD) << 32 - shiftamt) 			\
		    Ext_setone_low(extent);				\
                Variable_shift_double(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Qallp4(srcdstD));				\
                Variable_shift_double(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp3(srcdstC));				\
	     }								\
	     else {							\
		Extall(extent) = Qallp4(srcdstD);			\
		Qallp4(srcdstD) = Qallp3(srcdstC);			\
		Qallp3(srcdstC) = Qallp2(srcdstB);			\
	     }								\
	     Qallp2(srcdstB) = Qallp1(srcdstA) >> shiftamt;		\
	     Qallp1(srcdstA) = 0;					\
	     break;							\
     case 2: if (shiftamt > 0) {					\
                Variable_shift_double(Qallp2(srcdstB),Qallp3(srcdstC),	\
		 shiftamt,Extall(extent));				\
		if (Qallp4(srcdstD) || (Qallp3(srcdstC) << 32 - shiftamt)) \
		    Ext_setone_low(extent);				\
                Variable_shift_double(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Qallp4(srcdstD));				\
	     }								\
	     else {							\
		Extall(extent) = Qallp3(srcdstC);			\
		if (Qallp4(srcdstD)) Ext_setone_low(extent);		\
		Qallp4(srcdstD) = Qallp2(srcdstB);			\
	     }								\
	     Qallp3(srcdstC) = Qallp1(srcdstA) >> shiftamt;		\
	     Qallp1(srcdstA) = Qallp2(srcdstB) = 0;			\
	     break;							\
     case 3: if (shiftamt > 0) {					\
                Variable_shift_double(Qallp1(srcdstA),Qallp2(srcdstB),	\
		 shiftamt,Extall(extent));				\
		if (Qallp3(srcdstC) || Qallp4(srcdstD) || 		\
		    (Qallp2(srcdstB) << 32 - shiftamt))			\
		    Ext_setone_low(extent);				\
	     }								\
	     else {							\
		Extall(extent) = Qallp2(srcdstB);			\
		if (Qallp3(srcdstC) || Qallp4(srcdstD))			\
		    Ext_setone_low(extent);				\
	     }								\
	     Qallp4(srcdstD) = Qallp1(srcdstA) >> shiftamt;		\
	     Qallp1(srcdstA) = Qallp2(srcdstB) = Qallp3(srcdstC) = 0;	\
	     break;							\
     case 4: Extall(extent) = Qallp1(srcdstA);				\
	     if (Qallp2(srcdstB) || Qallp3(srcdstC) || Qallp4(srcdstD))	\
		 Ext_setone_low(extent);				\
	     Qallp1(srcdstA) = Qallp2(srcdstB) = 0;			\
	     Qallp3(srcdstC) = Qallp4(srcdstD) = 0;			\
	     break;							\
    }									\
  }

/* 
 * Here we need to shift the result right to correct for an overshift
 * (due to the exponent becoming negative) during normalization.
 */
#define Quad_fix_overshift(srcdstA,srcdstB,srcdstC,srcdstD,shift,extent) \
	    Extall(extent) = Qallp4(srcdstD) << 32 - (shift);		\
	    Qallp4(srcdstD) = (Qallp3(srcdstC) << 32 - (shift)) |	\
		(Qallp4(srcdstD) >> (shift));				\
	    Qallp3(srcdstC) = (Qallp2(srcdstB) << 32 - (shift)) |	\
		(Qallp3(srcdstC) >> (shift));				\
	    Qallp2(srcdstB) = (Qallp1(srcdstA) << 32 - (shift)) |	\
		(Qallp2(srcdstB) >> (shift));				\
	    Qallp1(srcdstA) = Qallp1(srcdstA) >> shift

#define Quad_hiddenhigh3mantissa(qval) Qhiddenhigh3mantissa(qval)
#define Quad_hidden(qval) Qhidden(qval)
#define Quad_lowmantissap4(qval) Qlowp4(qval)

/* The left argument is never smaller than the right argument */
#define Quad_subtract(lefta,leftb,leftc,leftd,righta,rightb,rightc,rightd,resulta,resultb,resultc,resultd) \
    if( Qallp4(rightd) > Qallp4(leftd) ) 			\
	if( (Qallp3(leftc)--) == 0)				\
	    if( (Qallp2(leftb)--) == 0) Qallp1(lefta)--;	\
    Qallp4(resultd) = Qallp4(leftd) - Qallp4(rightd);		\
    if( Qallp3(rightc) > Qallp3(leftc) ) 			\
        if( (Qallp2(leftb)--) == 0) Qallp1(lefta)--;		\
    Qallp3(resultc) = Qallp3(leftc) - Qallp3(rightc);		\
    if( Qallp2(rightb) > Qallp2(leftb) ) Qallp1(lefta)--;	\
    Qallp2(resultb) = Qallp2(leftb) - Qallp2(rightb);		\
    Qallp1(resulta) = Qallp1(lefta) - Qallp1(righta)

/* Subtract right augmented with extension from left augmented with zeros and
 * store into result and extension. */
#define Quad_subtract_withextension(lefta,leftb,leftc,leftd,righta,rightb,rightc,rightd,extent,resulta,resultb,resultc,resultd) \
    Quad_subtract(lefta,leftb,leftc,leftd,righta,rightb,rightc,rightd,	\
     resulta,resultb,resultc,resultd);					\
    if( (Extall(extent) = 0-Extall(extent)) )				\
        {								\
        if((Qallp4(resultd)--) == 0) 					\
            if((Qallp3(resultc)--) == 0) 				\
        	if((Qallp2(resultb)--) == 0) Qallp1(resulta)--;		\
        }

#define Quad_addition(lefta,leftb,leftc,leftd,righta,rightb,rightc,rightd,resulta,resultb,resultc,resultd) \
    /* If the sum of the low words is less than either source, then	\
     * an overflow into the next word occurred. */			\
    if ((Qallp4(resultd) = Qallp4(leftd) + Qallp4(rightd)) < Qallp4(rightd)) \
	if ((Qallp3(resultc) = Qallp3(leftc) + Qallp3(rightc) + 1) <= 	\
	    Qallp3(rightc)) 						\
	    if ((Qallp2(resultb) = Qallp2(leftb) + Qallp2(rightb) + 1)	\
	        <= Qallp2(rightb)) 					\
		    Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta) + 1; \
	    else Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta);	\
	else								\
	    if ((Qallp2(resultb) = Qallp2(leftb) + Qallp2(rightb)) <	\
	        Qallp2(rightb)) 					\
		    Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta) + 1; \
	    else Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta);	\
    else								\
	if ((Qallp3(resultc) = Qallp3(leftc) + Qallp3(rightc)) < 	\
	    Qallp3(rightc)) 						\
	    if ((Qallp2(resultb) = Qallp2(leftb) + Qallp2(rightb) + 1)	\
	        <= Qallp2(rightb)) 					\
		    Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta) + 1; \
	    else Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta);	\
	else								\
	    if ((Qallp2(resultb) = Qallp2(leftb) + Qallp2(rightb)) <	\
	        Qallp2(rightb)) 					\
		    Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta) + 1; \
	    else Qallp1(resulta) = Qallp1(lefta) + Qallp1(righta)

/* Need to Initialize */
#ifdef __hp9000s300
#define Quad_makequietnan(desta,destb,destc,destd)			\
    Qallp1(desta) = ((QUAD_EMAX+QUAD_BIAS)+1)<< (32-(1+QUAD_EXP_LENGTH)) \
                 | (1<<(32-(1+QUAD_EXP_LENGTH+1)));			\
    Qallp2(destb) = 0;							\
    Qallp3(destc) = 0;							\
    Qallp4(destd) = 0
#define Quad_makesignalingnan(desta,destb,destc,destd)			\
    Qallp1(desta) = ((QUAD_EMAX+QUAD_BIAS)+1)<< (32-(1+QUAD_EXP_LENGTH)) \
                 | (1<<(32-(1+QUAD_EXP_LENGTH+2)));			\
    Qallp2(destb) = 0;							\
    Qallp3(destc) = 0;							\
    Qallp4(destd) = 0
#else
#define Quad_makequietnan(desta,destb,destc,destd)			\
    Qallp1(desta) = ((QUAD_EMAX+QUAD_BIAS)+1)<< (32-(1+QUAD_EXP_LENGTH)) \
                 | (1<<(32-(1+QUAD_EXP_LENGTH+2)));			\
    Qallp2(destb) = 0;							\
    Qallp3(destc) = 0;							\
    Qallp4(destd) = 0
#define Quad_makesignalingnan(desta,destb,destc,destd)			\
    Qallp1(desta) = ((QUAD_EMAX+QUAD_BIAS)+1)<< (32-(1+QUAD_EXP_LENGTH)) \
                 | (1<<(32-(1+QUAD_EXP_LENGTH+1)));			\
    Qallp2(destb) = 0;							\
    Qallp3(destc) = 0;							\
    Qallp4(destd) = 0
#endif

#define Quad_normalize(qopndA,qopndB,qopndC,qopndD,exponent)		\
	while(Quad_iszero_hiddenhigh7mantissa(qopndA)) {		\
		Quad_leftshiftby8(qopndA,qopndB,qopndC,qopndD);		\
		exponent -= 8;						\
	}								\
	if(Quad_iszero_hiddenhigh3mantissa(qopndA)) {			\
		Quad_leftshiftby4(qopndA,qopndB,qopndC,qopndD);		\
		exponent -= 4;						\
	}								\
	while(Quad_iszero_hidden(qopndA)) {				\
		Quad_leftshiftby1(qopndA,qopndB,qopndC,qopndD);		\
		exponent -= 1;						\
	}

#define Fourword_add(src1dstA,src1dstB,src1dstC,src1dstD,src2A,src2B,src2C,src2D) \
	/* 								\
	 * want this macro to generate:					\
	 *	ADD	src1dstD,src2D,src1dstD;			\
	 *	ADDC	src1dstC,src2C,src1dstC;			\
	 *	ADDC	src1dstB,src2B,src1dstB;			\
	 *	ADDC	src1dstA,src2A,src1dstA;			\
	 */								\
	if ((src1dstD += (src2D)) < (src2D)) {				\
	    if ((src1dstC += (src2C) + 1) <= (src2C)) {			\
		if ((src1dstB += (src2B) + 1) <= (src2B)) src1dstA++;	\
	    }								\
	    else if ((src1dstB += (src2B)) < (src2B)) src1dstA++;	\
	}								\
	else {								\
	    if ((src1dstC += (src2C)) < (src2C)) {			\
		if ((src1dstB += (src2B) + 1) <= (src2B)) src1dstA++;	\
	    }								\
	    else if ((src1dstB += (src2B)) < (src2B)) src1dstA++;	\
	}								\
	src1dstA += (src2A)

#define Fourword_subtract(src1dstA,src1dstB,src1dstC,src1dstD,src2A,src2B,src2C,src2D) \
	/* 								\
	 * want this macro to generate:					\
	 *	SUB	src1dstD,src2D,src1dstD;			\
	 *	SUBB	src1dstC,src2C,src1dstC;			\
	 *	SUBB	src1dstB,src2B,src1dstB;			\
	 *	SUBB	src1dstA,src2A,src1dstA;			\
	 */								\
	if ((src1dstD) < (src2D)) 					\
	    if ((Qallp3(src1dstC)--) == 0)				\
	        if ((Qallp2(src1dstB)--) == 0) Qallp1(src1dstA)--;	\
	Qallp4(src1dstD) -= (src2D);					\
	if ((src1dstC) < (src2C)) 					\
	    if ((Qallp2(src1dstB)--) == 0) Qallp1(src1dstA)--;		\
	Qallp3(src1dstC) -= (src2C);					\
	if ((src1dstB) < (src2B)) Qallp1(src1dstA)--;			\
	Qallp2(src1dstB) -= (src2B);					\
	Qallp1(src1dstA) -= (src2A)

#define Quad_setoverflow(resultA,resultB,resultC,resultD)		\
	/* set result to infinity or largest number */			\
	switch (Rounding_mode()) {					\
		case ROUNDPLUS:						\
			if (Quad_isone_sign(resultA)) {			\
				Quad_setlargestnegative(resultA,	\
				 resultB,resultC,resultD);		\
			}						\
			else {						\
				Quad_setinfinitypositive(resultA,	\
				 resultB,resultC,resultD); 		\
			}						\
			break;						\
		case ROUNDMINUS:					\
			if (Quad_iszero_sign(resultA)) {		\
				Quad_setlargestpositive(resultA,	\
				 resultB,resultC,resultD);		\
			}						\
			else {						\
				Quad_setinfinitynegative(resultA,	\
				 resultB,resultC,resultD);		\
			}						\
			break;						\
		case ROUNDNEAREST:					\
			Quad_setinfinity_exponentmantissa(resultA,	\
			 resultB,resultC,resultD); 			\
			break;						\
		case ROUNDZERO:						\
			Quad_setlargest_exponentmantissa(resultA,	\
			 resultB,resultC,resultD); 			\
	}

#define Quad_denormalize(opndp1,opndp2,opndp3,opndp4,exponent,guard,sticky,inexact) \
  {int shiftamt;							\
    Quad_clear_signexponent_set_hidden(opndp1);				\
    if (exponent >= (1-QUAD_P)) {					\
	shiftamt = (1-exponent) % 32;					\
	switch((1-exponent)/32) {					\
	  case 0: Variableshiftdouble(opndp4,0,shiftamt,inexact);	\
		  guard = inexact >> 31;				\
		  sticky |= inexact << 1;				\
		  Variableshiftdouble(opndp3,opndp4,shiftamt,opndp4);	\
		  Variableshiftdouble(opndp2,opndp3,shiftamt,opndp3);	\
		  Variableshiftdouble(opndp1,opndp2,shiftamt,opndp2);	\
		  Qallp1(opndp1) >>= shiftamt;				\
		  break;						\
	  case 1: Variableshiftdouble(opndp3,opndp4,shiftamt,inexact);	\
		  guard = inexact >> 31;				\
		  sticky |= (inexact << 1) | (Qallp4(opndp4) << 1);	\
		  Variableshiftdouble(opndp2,opndp3,shiftamt,opndp4);	\
		  Variableshiftdouble(opndp1,opndp2,shiftamt,opndp3);	\
		  Qallp2(opndp2) = Qallp1(opndp1) >> shiftamt;		\
		  Qallp1(opndp1) = 0;					\
		  break;						\
	  case 2: Variableshiftdouble(opndp2,opndp3,shiftamt,inexact);	\
		  guard = inexact >> 31;				\
		  sticky |= (inexact << 1) | (Qallp3(opndp3) << 1) | 	\
			    Qallp4(opndp4);				\
		  Variableshiftdouble(opndp1,opndp2,shiftamt,opndp4);	\
		  Qallp3(opndp3) = Qallp1(opndp1) >> shiftamt;		\
		  Qallp1(opndp1) = Qallp2(opndp2) = 0;			\
		  break;						\
	  case 3: Variableshiftdouble(opndp1,opndp2,shiftamt,inexact);	\
		  guard = inexact >> 31;				\
		  sticky |= (inexact << 1) | (Qallp2(opndp2) << 1) |	\
		   Qallp3(opndp3) | Qallp4(opndp4);			\
		  Qallp4(opndp4) = Qallp1(opndp1) >> shiftamt;		\
		  Qallp1(opndp1) = Qallp2(opndp2) = Qallp3(opndp3) = 0;	\
		  break;						\
	}								\
	inexact = guard | sticky;					\
    }									\
    else {								\
	guard = 0;							\
	sticky |= (Qallp1(opndp1) | Qallp2(opndp2));			\
	Quad_setzero(opndp1,opndp2,opndp3,opndp4);			\
	inexact = sticky;						\
    }									\
  }


/************************************
 * Declare quad precision functions *
 *  used by the convert routines    *
 ************************************/

#define Qintp1(object) (object)
#define Qintp2(object) (object)
#define Qintp3(object) (object)
#define Qintp4(object) (object)

/*
 * Quad format macros
 */

#define Qint_isinexact_to_sgl(qintA,qintB,qintC,qintD)		\
    ((Qintp1(qintA) << 33 - SGL_EXP_LENGTH) || Qintp2(qintB) || \
     Qintp3(qintC) || Qintp4(qintD))

#define Sgl_roundnearest_from_qint(qintA,qintB,qintC,qintD,sgl_opnd)	\
    if (Qintp1(qintA) & 1 << (SGL_EXP_LENGTH - 2 ))			\
       if ((Qintp1(qintA) << 34 - SGL_EXP_LENGTH) || Qintp2(qintB) ||	\
	   Qintp3(qintC) || Qintp4(qintD) || Slow(sgl_opnd))		\
	       Sall(sgl_opnd)++

#define Qint_isinexact_to_dbl(qintB,qintC,qintD)	\
    ((Qintp2(qintB) << 33 - DBL_EXP_LENGTH) || Qintp3(qintC) || Qintp4(qintD))

#define Dbl_roundnearest_from_qint(qintB,qintC,qintD,dbl_opndA,dbl_opndB) \
    if (Qintp2(qintB) & 1 << (DBL_EXP_LENGTH - 2))			\
       if ((Qintp2(qintB) << 34 - DBL_EXP_LENGTH) || Qintp3(qintC) ||	\
	   Qintp4(qintD) || Dlowp2(dbl_opndB))				\
	      if ((++Dallp2(dbl_opndB)) == 0) Dallp1(dbl_opndA)++

#define Qint_isinexact_to_quad(qintD)	\
    (Qintp4(qintD) << 33 - QUAD_EXP_LENGTH)

#define Quad_roundnearest_from_qint(qintD,quadA,quadB,quadC,quadD)	\
    if (Qintp4(qintD) & 1 << (QUAD_EXP_LENGTH - 2))			\
       if ((Qintp4(qintD) << 34 - QUAD_EXP_LENGTH) || Qlowp4(quadD))	\
	  if ((++Qallp4(quadD)) == 0)					\
	     if ((++Qallp3(quadC)) == 0)				\
	        if ((++Qallp2(quadB)) == 0) Qallp1(quadA)++

#define Sgl_to_quad_exponent(src_exponent,dest)			\
    Deposit_qexponent(dest,src_exponent+(QUAD_BIAS-SGL_BIAS))

#define Sgl_to_quad_mantissa(src_mantissa,destA,destB,destC,destD) \
    Deposit_qmantissap1(destA,src_mantissa>>7);			\
    Qmantissap2(destB) = src_mantissa << 25;			\
    Qmantissap3(destC) = 0; Qmantissap4(destD) = 0

#define Dbl_to_quad_exponent(src_exponent,dest)			\
    Deposit_qexponent(dest,src_exponent+(QUAD_BIAS-DBL_BIAS))

#define Dbl_to_quad_mantissa(src_mantissap1,srcp2,destA,destB,destC,destD) \
    Deposit_qmantissap1(destA,src_mantissap1>>4);		\
    Qmantissap2(destB) = (src_mantissap1 << 28) | (srcp2 >> 4);	\
    Qmantissap3(destC) = srcp2 << 28;				\
    Qmantissap4(destD) = 0

#define Quad_to_sgl_exponent(src_exponent,dest)			\
    dest = src_exponent + (SGL_BIAS - QUAD_BIAS)

#define Quad_to_sgl_mantissa(srcA,srcB,srcC,srcD,dest,inexact,guard,sticky,odd)\
    Shiftdouble(Qmantissap1(srcA),Qmantissap2(srcB),25,dest); 	\
    guard = Qbit7p2(srcB);					\
    sticky = (Qallp2(srcB)<<8) | Qallp3(srcC) | Qallp4(srcD);	\
    inexact = guard | sticky;					\
    odd = Qbit6p2(srcB)

#define Quad_to_sgl_denormalized(srcA,srcB,srcC,srcD,exp,dest,inexact,guard,sticky,odd) \
    Deposit_qsignexponent(srcA,1);					\
    Shiftdouble(srcA,srcB,24,srcA);					\
    sticky = (Qallp2(srcB) << 8) | Qallp3(srcC) | Qallp4(srcD);		\
    if (exp < (1 - SGL_P)) {						\
	dest = 0;							\
    	guard = 0;							\
    	sticky |= Qallp1(srcA);						\
    }									\
    else {								\
    	dest = Qallp1(srcA) >> (2 - exp);				\
    	inexact = Qallp1(srcA) << (30 + exp);				\
    	guard = inexact >> 31;						\
    	sticky |= inexact << 1;						\
    }									\
    odd = dest << 31;							\
    inexact = guard | sticky; 						\
    exp = 0 

#define Quad_to_dbl_exponent(src_exponent,dest)			\
    dest = src_exponent + (DBL_BIAS - QUAD_BIAS)

#define Quad_to_dbl_mantissa(srcA,srcB,srcC,srcD,destA,destB,inexact,guard,sticky,odd) \
    Shiftdouble(Qmantissap1(srcA),Qmantissap2(srcB),28,destA); 	\
    Shiftdouble(Qmantissap2(srcB),Qmantissap3(srcC),28,destB); 	\
    guard = Qbit4p3(srcC);					\
    sticky = (Qallp3(srcC)<<5) | Qallp4(srcD);			\
    inexact = guard | sticky;					\
    odd = Qbit3p3(srcC)

#define Quad_to_dbl_denormalized(srcA,srcB,srcC,srcD,exp,destA,destB,inexact,guard,sticky,odd) \
    Deposit_qsignexponent(srcA,1);					\
    Shiftdouble(srcA,srcB,27,srcA);					\
    Shiftdouble(srcB,srcC,27,srcB);					\
    sticky = (Qallp3(srcC) << 5) | Qallp4(srcD);			\
    if (exp < -30) {							\
	destA = 0;							\
	sticky |= Qallp2(srcB);						\
        if (exp < (1 - DBL_P)) {					\
	    destB = 0;							\
    	    guard = 0;							\
    	    sticky |= Qallp1(srcA);					\
        }								\
	else {								\
	    destB = Qallp1(srcA) >> (-30 - exp);			\
	    inexact = Qallp1(srcA) << (62 + exp);			\
	    guard = inexact >> 31;					\
	    sticky |= inexact << 1;					\
	}								\
    }									\
    else {								\
        if (exp == -30) {						\
	    destA = 0;							\
	    destB = Qallp1(srcA);					\
	    guard = Qallp2(srcB) >> 31;					\
	    sticky |= (Qallp2(srcB) << 1);				\
	}								\
	else {								\
	    destA = Qallp1(srcA) >> (2 - exp);				\
	    Variable_shift_double(srcA,srcB,2-exp,destB);		\
	    inexact = Qallp2(srcB) << (30 + exp);			\
	    guard = inexact >> 31;					\
	    sticky |= inexact << 1;					\
	}								\
    }									\
    odd = destB << 31;							\
    inexact = guard | sticky; 						\
    exp = 0

#define Quad_isinexact_to_fix(opndA,opndB,opndC,opndD,exp)		\
    (exp < (QUAD_P-97) ? 						\
       Qallp4(opndD) || Qallp3(opndC) || Qallp2(opndB) || 		\
        Qallp1(opndA) << (QUAD_EXP_LENGTH+1+exp) : 			\
      (exp < (QUAD_P-65) ? 						\
         Qallp4(opndD) || Qallp3(opndC) || 				\
	  Qallp2(opndB) << (exp + (97-QUAD_P)) :   			\
        (exp < (QUAD_P-33) ? 						\
           Qallp4(opndD) || Qallp3(opndC) << (exp + (65-QUAD_P)) :	\
          (exp < (QUAD_P-1) ? Qallp4(opndD) << (exp + (33-QUAD_P)) : FALSE))))

#define Quad_isoverflow_to_int(exp,opndA,opndB)			\
    ((exp > SGL_FX_MAX_EXP + 1) || Qsign(opndA)==0 ||		\
     Qmantissap1(opndA)!=0 || (Qallp2(opndB)>>17)!=0 )

#define Quad_isoverflow_to_unsigned(exp,opndA,opndB)		\
    ((exp > SGL_FX_MAX_EXP + 2) || Qsign(opndA)==0 ||		\
     Qmantissap1(opndA)!=0 || (Qallp2(opndB)>>16)!=0 )

#define Quad_isoverflow_to_dint(exp,opndA,opndB,opndC)		\
    ((exp > DBL_FX_MAX_EXP + 1) || Qsign(opndA)==0 ||		\
     Qmantissap1(opndA)!=0 || Qallp2(opndB) || (Qallp3(opndC)>>17)!=0 )

#define Quad_isone_roundbit(opndA,opndB,opndC,opndD,exp)	\
    ((exp < (QUAD_P - 97) ?					\
        Qallp1(opndA) >> ((QUAD_P - 98) - exp) :		\
       (exp < (QUAD_P - 65) ?					\
          Qallp2(opndB) >> ((QUAD_P - 66) - exp) :		\
         (exp < (QUAD_P - 33) ?					\
            Qallp3(opndC) >> ((QUAD_P - 34) - exp) :		\
            Qallp4(opndD) >> ((QUAD_P - 2) - exp)))) & 1)

#define Quad_isone_stickybit(opndA,opndB,opndC,opndD,exp)		\
    (exp < (QUAD_P-98) ? 						\
       Qallp4(opndD) || Qallp3(opndC) || Qallp2(opndB) || 		\
        Qallp1(opndA) << (QUAD_EXP_LENGTH+2+exp) : 			\
      (exp < (QUAD_P-66) ? 						\
         Qallp4(opndD) || Qallp3(opndC) || 				\
	  Qallp2(opndB) << (exp + (98-QUAD_P)) :   			\
        (exp < (QUAD_P-34) ? 						\
           Qallp4(opndD) || Qallp3(opndC) << (exp + (66-QUAD_P)) :	\
          (exp < (QUAD_P-2) ? Qallp4(opndD) << (exp + (34-QUAD_P)) : FALSE))))


/* Int macros */

#define Int_from_quad_mantissa(qvalueA,qvalueB,exponent)		\
    Shiftdouble(Qallp1(qvalueA),Qallp2(qvalueB),18,Qallp1(qvalueA));	\
    if (exponent < 31) Qallp1(qvalueA) >>= 30 - exponent;		\
    else Qallp1(qvalueA) <<= 1


/* Dint macros */

#define Dint_from_quad_mantissa(srcA,srcB,srcC,exponent,destA,destB) \
    {if (exponent < 32) {						\
    	Dintp1(destA) = 0;						\
    	if (exponent <= 16)						\
    	    Dintp2(destB) = Qallp1(srcA) >> 16-exponent;		\
    	else Variable_shift_double(Qallp1(srcA),Qallp2(srcB),48-exponent, \
	 Dintp2(destB));						\
    }									\
    else {								\
    	if (exponent <= 48) {						\
    	    Dintp1(destA) = Qallp1(srcA) >> 48-exponent;		\
	    if (exponent == 48) Dintp2(destB) = Qallp2(srcB);		\
	    else Variable_shift_double(Qallp1(srcA),Qallp2(srcB),48-exponent, \
	     Dintp2(destB));						\
        }								\
    	else {								\
    	    Variable_shift_double(Qallp1(srcA),Qallp2(srcB),80-exponent, \
	     Dintp1(destA));						\
    	    Variable_shift_double(Qallp2(srcB),Qallp3(srcC),80-exponent, \
	     Dintp2(destB));						\
    	}								\
    }}


/* Qint macros */

#define Qint_from_sgl_mantissa(src,exp,destA,destB,destC,destD)		\
    {Sall(src) <<= SGL_EXP_LENGTH;  /*  left-justify  */		\
    if (exp <= 63) {							\
	if (exp <= 31) {						\
    	    Qintp1(destA) = 0;						\
    	    Qintp2(destB) = 0;						\
    	    Qintp3(destC) = 0;						\
    	    Qintp4(destD) = (unsigned)Sall(src) >> (31 - exp); 		\
	}								\
	else {								\
    	    Qintp1(destA) = 0;						\
    	    Qintp2(destB) = 0;						\
    	    Qintp3(destC) = (unsigned)Sall(src) >> (63 - exp);		\
	    Variable_shift_double(Sall(src),0,63-exp,Qintp4(destD));	\
	}								\
    }									\
    else if (exp <= 95) {						\
    	Qintp1(destA) = 0;						\
    	Qintp2(destB) = (unsigned)Sall(src) >> (95 - exp);		\
	Variable_shift_double(Sall(src),0,95-exp,Qintp3(destC));	\
    	Qintp4(destD) = 0;						\
    }									\
    else {								\
    	Qintp1(destA) = (unsigned)Sall(src) >> (127 - exp);		\
	Variable_shift_double(Sall(src),0,127-exp,Qintp2(destB));	\
    	Qintp3(destC) = 0;						\
    	Qintp4(destD) = 0;						\
    }}


#define Qint_from_dbl_mantissa(srcA,srcB,exp,destA,destB,destC,destD)	\
    {int shiftamt;							\
    Shiftdouble(Dallp1(srcA),Dallp2(srcB),21,Dallp1(srcA));		\
    Dallp2(srcB) <<= 11;						\
    shiftamt = 31 - (exp % 32);						\
    switch (exp/32) {							\
     case 0: Qintp1(destA) = 0;						\
	     Qintp2(destB) = 0;						\
	     Qintp3(destC) = 0;						\
	     Qintp4(destD) = Dallp1(srcA) >> shiftamt;			\
	     break;							\
     case 1: Qintp1(destA) = 0;						\
	     Qintp2(destB) = 0;						\
	     Qintp3(destC) = Dallp1(srcA) >> shiftamt;			\
	     Variableshiftdouble(Dallp1(srcA),Dallp2(srcB),shiftamt,	\
	       Qintp4(destD));						\
	     break;							\
     case 2: Qintp1(destA) = 0;						\
	     Qintp2(destB) = Dallp1(srcA) >> shiftamt;			\
	     Variableshiftdouble(Dallp1(srcA),Dallp2(srcB),shiftamt,	\
	       Qintp3(destC));						\
	     Variableshiftdouble(Dallp2(srcB),0,shiftamt,Qintp4(destD)); \
	     break;							\
     case 3: Qintp1(destA) = Dallp1(srcA) >> shiftamt;			\
	     Variable_shift_double(Dallp1(srcA),Dallp2(srcB),shiftamt,	\
	       Qintp2(destB));						\
	     Variable_shift_double(Dallp2(srcB),0,shiftamt,Qintp3(destC)); \
	     Qintp4(destD) = 0;						\
	     break;							\
    }}

#define Qint_from_quad_mantissa(exp,opndA,opndB,opndC,opndD) 		\
    {int shiftamt;							\
    Shiftdouble(Qallp1(opndA),Qallp2(opndB),17,Qallp1(opndA));		\
    Shiftdouble(Qallp2(opndB),Qallp3(opndC),17,Qallp2(opndB));		\
    Shiftdouble(Qallp3(opndC),Qallp4(opndD),17,Qallp3(opndC));		\
    Qallp4(opndD) <<= 15;						\
    shiftamt = 31 - (exp % 32);						\
    switch (exp/32) {							\
     case 0: Qintp4(opndD) = (unsigned)Qallp1(opndA) >> shiftamt;	\
	     Qintp3(opndC) = 0;						\
	     Qintp2(opndB) = 0;						\
	     Qintp1(opndA) = 0;						\
	     break;							\
     case 1: Variableshiftdouble(Qallp1(opndA),Qallp2(opndB),shiftamt,	\
	       Qintp4(opndD));						\
	     Qintp3(opndC) = (unsigned)Qallp1(opndA) >> shiftamt;	\
	     Qintp2(opndB) = 0;						\
	     Qintp1(opndA) = 0;						\
	     break;							\
     case 2: Variableshiftdouble(Qallp2(opndB),Qallp3(opndC),shiftamt,	\
	       Qintp4(opndD));						\
	     Variableshiftdouble(Qallp1(opndA),Qallp2(opndB),shiftamt,	\
	       Qintp3(opndC));						\
	     Qintp2(opndB) = (unsigned)Qallp1(opndA) >> shiftamt;	\
	     Qintp1(opndA) = 0;						\
	     break;							\
     case 3: Variableshiftdouble(Qallp3(opndC),Qallp4(opndD),shiftamt,	\
	       Qintp4(opndD));						\
	     Variableshiftdouble(Qallp2(opndB),Qallp3(opndC),shiftamt,	\
	       Qintp3(opndC));						\
	     Variableshiftdouble(Qallp1(opndA),Qallp2(opndB),shiftamt,	\
	       Qintp2(opndB));						\
	     Qintp1(opndA) = (unsigned)Qallp1(opndA) >> shiftamt;	\
	     break;							\
    }}


#define Qint_setzero(destA,destB,destC,destD) 	\
    Qintp1(destA) = 0; 	\
    Qintp2(destB) = 0; 	\
    Qintp3(destC) = 0; 	\
    Qintp4(destD) = 0

#define Qint_setone_sign(destA,destB,destC,destD)	\
    Qintp1(destA) = ~Qintp1(destA);			\
    Qintp2(destB) = ~Qintp2(destB);			\
    Qintp3(destC) = ~Qintp3(destC);			\
    if ((Qintp4(destD) = -Qintp4(destD)) == 0)		\
    	if (++Qintp3(destC) == 0)			\
    	    if (++Qintp2(destB) == 0) Qintp1(destA)++

#define Qint_set_minint(destA,destB,destC,destD) \
    Qintp1(destA) = 1<<31;	\
    Qintp2(destB) = 0;		\
    Qintp3(destC) = 0;		\
    Qintp4(destD) = 0

#define Qint_isone_lowp4(destD)  (Qintp4(destD) & 01)

#define Qint_increment(destA,destB,destC,destD)		\
    if ((++Qintp4(destD))==0)				\
	if ((++Qintp3(destC))==0)			\
    	    if ((++Qintp2(destB))==0) Qintp1(destA)++

#define Qint_decrement(destA,destB,destC,destD)		\
    if ((Qintp4(destD)--)==0) 				\
    	if ((Qintp3(destC)--)==0) 			\
    	    if ((Qintp2(destB)--)==0) Qintp1(destA)--

#define Qint_negate(destA,destB,destC,destD)		\
    Qintp1(destA) = ~Qintp1(destA);			\
    Qintp2(destB) = ~Qintp2(destB);			\
    Qintp3(destC) = ~Qintp3(destC);			\
    if ((Qintp4(destD) = -Qintp4(destD)) == 0)		\
    	if (++Qintp3(destC) == 0) 			\
    	    if (++Qintp2(destB) == 0) Qintp1(destA)++

#define Qint_copyfromptr(src,destA,destB,destC,destD) \
     Qintp1(destA) = src->wd0;		\
     Qintp2(destB) = src->wd1;		\
     Qintp3(destC) = src->wd2;		\
     Qintp4(destD) = src->wd3

#define Qint_copytoptr(srcA,srcB,srcC,srcD,dest) \
    dest->wd0 = Qintp1(srcA);		\
    dest->wd1 = Qintp2(srcB);		\
    dest->wd2 = Qintp3(srcC);		\
    dest->wd3 = Qintp4(srcD)

