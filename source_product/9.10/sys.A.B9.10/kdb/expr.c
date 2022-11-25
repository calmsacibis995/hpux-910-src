/* @(#) $Revision: 66.2 $ */    
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines do expression parsing and evaluation.	 TkFExpr() is the
 * real starting point; it uses the other routines to parse a line, build
 * push-down operand and operator stacks, and pop and evaluate subexpressions
 * them as needed (as lower-precedence operators are encountered).
 *
 * An operand consists of an address (which may be an immediate, constant
 * value) and accompanying type information that tells how to interpret it.
 */

#include <ctype.h>
#include "cdb.h"
#include "macdefs.h"

extern	long	atol();
#ifdef KFLPT
extern	double	atof();
#endif
extern	int	vrgOffset[];		/* defined in access.c */

local	char	*sbErrExpr = "Misformed expression";
local	char	vsbExprSave [80];	/* for error reporting */

#define	ExprTkNext()	{ strcat (vsbExprSave, vsbTok); TkNext(); }

local	STUFFU	astuffZeros;		/* defaults to all zeroes */

#define	FUnsigned(bt)	\
    ((bt == btUChar) OR (bt == btUShort) OR (bt == btUInt) OR (bt == btULong))


/***********************************************************************
 * STACK MECHANISM used by the expression parser:
 *
 * The operator stack is just an array of tokens.
 * The operand (variable) stack is a pair of arrays, of types and
 * addresses.  Immediate values (constants) are stored in the addresses
 * except that constant doubles are stored in extension TYR records.
 */

export	int	viopMac;		/* operator stack size */
export	int	vivarMac;		/* variable stack size */

#define istkMax 15			/* how deep they can get */

TKE	vrgTk	   [istkMax];		/* operator stack	 */
TYR	vrgTy	   [istkMax * cTyMax];	/* variable stack	 */
long	vrgAdrLong [istkMax * cTyMax];	/* adrs of vars in vrgTy */

#define TOPOP	(vrgTk [viopMac	- 1])	/* assumes stack not empty */


/***********************************************************************
 * PARSER STATE:
 */

#define psFail	0			/* parser confused		*/
#define psOp	1			/* parser last saw an OPERATOR	*/
#define psVar	2			/* parser last saw a  VARIABLE	*/
#define psParam	3			/* parser finished a  PARAMETER	*/

typedef int  PSE, *pPSE;


/***********************************************************************
 * E X P R   E R R O R
 *
 * This front-end for UError() prints a message with possible args,
 * then tries to show where the error occurred in the expression, then
 * calls UError() to finish up and bail out.  Assumes vsbExprSave and
 * vsbTok are valid!
 */

/* VARARGS1 */
local void ExprError (msg, arg1, arg2, arg3, arg4)
    char	*msg;			/* message (printf() string) */
    int		arg1, arg2, arg3, arg4; /* arguments that go with it */
{
    printf (msg, arg1, arg2, arg3, arg4);

    /*
     * Finish line with colon, show expr so far plus vtk, then prepare
     * the next line to mark the start of the "bad" (current) token:
     */
    printf (":\n%s%s\n%*s", vsbExprSave, vsbTok, strlen (vsbExprSave), "");

    viopMac = 0;		/* clean up the stacks */
    vivarMac = 0;
    UError ("^");		/* mark under the token */

} /* ExprError */


/***********************************************************************
 * P U S H   O P
 *
 * Push an operator (token type) on the op stack.
 * Overflow is not expected, so it's checked here.
 */

local void PushOp (tk)
    TKE		tk;		/* token type of operator to push */
{
    if (viopMac >= istkMax)
	ExprError ("Operator stack overflow");

    vrgTk [viopMac++] = tk;

} /* PushOp */


/***********************************************************************
 * P O P   O P
 *
 * Pop an operator (token type) from the op stack.
 * Underflow is not expected, so it's checked here.
 */

local TKE PopOp()
{
    if (viopMac <= 0)
	ExprError ("Operator stack underflow");

    return (vrgTk [--viopMac]);

} /* PopOp */


/***********************************************************************
 * P U S H    V A R
 *
 * Push a variable (adr and type) on the operand stack (arrays).
 * Overflow is not expected, so it's checked here.
 */

local void PushVar (adrLong, ty)
    long	adrLong;	/* adr	of var to push */
    pTYR	ty;		/* type of var to push */
{
    if (vivarMac >= istkMax)
	ExprError ("Operand stack overflow");

    CopyTy (vrgTy + vivarMac, ty);
    vrgAdrLong [vivarMac] = adrLong;

    vivarMac += cTyMax;			/* each ty is cTyMax records */

} /* PushVar */


/***********************************************************************
 * F   P O P   V A R
 *
 * Pop a variable (adr and type) from the operand stack (arrays).
 * Underflow is an "expected condition" so we let the caller handle it.
 */

local FLAGT FPopVar (padr, ty)
    long	*padr;		/* where to save var adr  */
    pTYR	ty;		/* where to save var type */
{
    if (vivarMac <= 0)
	return (false);			/* nothing left to pop */

    vivarMac -= cTyMax;			/* each ty is cTyMax records */
    *padr = vrgAdrLong [vivarMac];
    CopyTy (ty, vrgTy + vivarMac);
    return (true);

} /* FPopVar */




/***********************************************************************
 * P R E C   F	 T K
 *
 * Determine operator precedence.  These values are meaningful only in
 * their relationship to one another.  See the K&R (the C language
 * reference), pp 214-215.
 * 
 * If a token has no precedence (if this routine returns zero), the token
 * is not an operator.  For paired tokens (such as "[]"), the left token's
 * precedence must be >= the right token's.  Note that tkBOP and tkComma
 * are a pair.
 *
 * tkIndex is a very special case which must have higher priority than
 * ANYTHING, so it is executed as soon as it is on top of the stack (e.g.
 * when the "]" comes along and "[" is popped).  (tkIndex is a pseudo-op
 * pushed when the "[" is seen.)
 */

local int PrecFTk (tk)
    TKE		tk;		/* the token (operator) to do */
{
    switch (tk)
    {
	default:	return (0);

	case tkIndex:	return (110);

	case tkBOE:			/* primary-expressions */
	case tkBOP:
	case tkLP:
	case tkLSB:
	case tkDot:
	case tkPtr:	return (100);

	case tkDeref:			/* unary operators */
	case tkDerefP:
	case tkRef:
	case tkUMinus:
	case tkBang:
	case tkTilda:
	case tkPlusPlus:
	case tkMinusMinus:
	case tkSizeof:
	case tkInside:	return (90);

	case tkMul:			/* various math and logical ops */
	case tkDiv:
	case tkModulo:	return (80);

	case tkPlus:
	case tkMinus:	return (70);

	case tkRShift:
	case tkLShift:	return (60);

	case tkLT:
	case tkGT:
	case tkLE:
	case tkGE:	return (50);

	case tkEqual:
	case tkNotEqual: return (40);

	case tkBitAnd:	return (32);
	case tkXOR:	return (31);
	case tkBitOr:	return (30);

	case tkLAND:	return (21);
	case tkLOR:	return (20);

     /* case tkCondition: return (10);	   missing "?:" */

	case tkAssign:			/* assignment ops */
	case tkAssPlus:
	case tkAssMinus:
	case tkAssMult:
	case tkAssDiv:
	case tkAssMod:
	case tkAssRight:
	case tkAssLeft:
	case tkAssBAND:
	case tkAssXOR:
	case tkAssBOR:	return (5);

	case tkEOE:			/* right parts of primary-expressions */
	case tkRP:
	case tkRSB:
	case tkComma:	return (1);

    } /* switch */

} /* PrecFTk */


/***********************************************************************
 * F   L E F T   A S S O C
 *
 * Determine whether an operator is left-associative.
 * See the K&R (the C language reference), pp 214-215.
 * 
 * This routine returns true for left-associative operators and false
 * for right-associative operators.  Non-operators return true, so this
 * routine should not be used to test for operator status.
 */

local int FLeftAssoc (tk)
    TKE		tk;		/* the token (operator) to do */
{
    switch (tk)
    {
	default:	return (true);

	case tkIndex:			/* special case		*/
	case tkBOE:			/* primary-expressions	*/
	case tkBOP:
	case tkLP:
	case tkLSB:
	case tkDot:
	case tkPtr:	return (true);

	case tkDeref:			/* unary operators */
	case tkDerefP:
	case tkRef:
	case tkUMinus:
	case tkBang:
	case tkTilda:
	case tkPlusPlus:
	case tkMinusMinus:
	case tkSizeof:
	case tkInside:	return (false);

	case tkMul:			/* various math and logical ops */
	case tkDiv:
	case tkModulo:
	case tkPlus:
	case tkMinus:
	case tkRShift:
	case tkLShift:
	case tkLT:
	case tkGT:
	case tkLE:
	case tkGE:
	case tkEqual:
	case tkNotEqual:
	case tkBitAnd:
	case tkXOR:
	case tkBitOr:
	case tkLAND:
	case tkLOR:

     /* case tkCondition: 	   missing "?:" */

			return (true);

	case tkAssign:			/* assignment ops */
	case tkAssPlus:
	case tkAssMinus:
	case tkAssMult:
	case tkAssDiv:
	case tkAssMod:
	case tkAssRight:
	case tkAssLeft:
	case tkAssBAND:
	case tkAssXOR:
	case tkAssBOR:	return (false);

	case tkEOE:			/* right parts of "operator" pairs */
	case tkRP:
	case tkRSB:
	case tkComma:	return (true);

    } /* switch */

} /* FLeftAssoc */

#ifdef JUNK
/***********************************************************************
 * F   N U M   F   T Y
 *
 * Tell if a type can be treated as an arithmetic operand (a number).
 */

local FLAGT FNumFTy (ty)
    pTYR	ty;		/* type to check */
{
    if (TqFTy (ty, 1) != tqNil)		/* pointer, array, or func */
	return (true);

    switch (ty->td.bt)
    {
	default:
	case btNil:		/* true for HPSYMTAB(II) extended types too */
	case btEType:
	case btUnion:
	case btStruct:	return (false);

	case btFArg:				/* what's an FArg, anyway? */
	case btChar:
	case btShort:
	case btInt:
	case btLong:
	case btFloat:
	case btDouble:
	case btUChar:
	case btUShort:
	case btUInt:
	case btULong:
	case btEMember: return (true);

    } /* switch */

} /* FNumFTy */
#endif /* JUNK */




/***********************************************************************
 * C O N V   T Y P E
 *
 * Convert a value from a current type to a desired type.  Only does the
 * conversion if the current type is a simple basetype and the desired
 * type is a simple basetype or simple array (so as to store correctly
 * into an element), and then only if conversion is needed.
 */

export void ConvType (pstuff, tyWant, tyHave)
    register STUFFU	*pstuff;		/* value to convert */
    pTYR	tyWant;				/* desired type	    */
    pTYR	tyHave;				/* current type	    */
{
    register int	itq;			/* tq index	  */
    register int	tq;			/* tq value	  */
    int		btWant	  = tyWant->td.bt;	/* base types	  */
    int		btHave	  = tyHave->td.bt;
    FLAGT	fUnsigned = FUnsigned (btHave);	/* careful, it's a macro */
    FLAGT	fFloat	  = (btHave == btFloat);
    FLAGT	fDouble	  = (btHave == btDouble);
    ulong	valu	  = pstuff->lng;	/* unsigned value */

/*
 * CHECK FOR SIMPLE TYPES:
 */

    if (TqFTy (tyHave, 1) != tqNil)		/* current is not simple */
	return;

    for (itq = 1; itq <= 6; itq++)
	if ((tq = TqFTy (tyWant, itq)) != tqArray)
	    break;

    if ((itq <= 6) AND (tq != tqNil))		/* desired is not simple */
	return;

/*
 * DO TYPE CONVERSION IF NEEDED:
 */

    if (btWant == btDouble)			/* DOUBLE = (something) */
    {
	if (! fDouble)				/* not from double */
	    pstuff->doub = fFloat    ? pstuff->fl :
			   fUnsigned ? valu	  :
				       pstuff->lng;
    }
    else if (btWant == btFloat)			/* FLOAT = (something) */
    {
	if (! fFloat)				/* not from float */
	    pstuff->fl = fDouble   ? pstuff->doub :
			 fUnsigned ? valu	  :
				     pstuff->lng;
    }
    else if (fDouble)				/* NON-REAL = DOUBLE */
    {
	pstuff->lng = pstuff->doub;
    }
    else if (fFloat)				/* NON-REAL = FLOAT */
    {
	pstuff->lng = pstuff->fl;
    }
    /* else no conversion needed */

} /* ConvType */


/***********************************************************************
 * F   V A L   F   T O K
 *
 * Tell if the current vsbTok is a pure number and if so, return its
 * value, cast to a long if necessary.  Always returns a long, even for
 * floating point values.  Advances the current token to a following "L"
 * or "l" if any, so as to eat it.  Most callers always want a long; only
 * some care about the trailing "L" or "l".
 */

export FLAGT FValFTok (pval)
    long	*pval;		/* where to return the value */
{
    switch (vtk)
    {
	default: return (false);

	case tkHex:
		sscanf ((vsbTok + 2), "%X", pval);
		break;

	case tkDecimal:
		*pval = atol (vsbTok);
		break;

	case tkOctal:
		sscanf (vsbTok, "%O", pval);
		break;

#ifdef KFLPT
	case tkFloat:
		*pval = (long) atof (vsbTok);
		break;
#endif

    } /* switch */

    return (true);

} /* FValFTok */


/***********************************************************************
 * V A L   F   A D R   T Y
 *
 * Given an address, type, and optional size, return the data pointed to
 * by that address, or the address itself if the type is a constant or
 * "special" (or the double value for double constants), sign-extended
 * or cast to long if necessary.
 * 
 * Values shorter than double are "masked" in the sense that only the
 * needed bytes are gotten (except for constants, which must already have
 * the right value).  Follows the rules of C for lengthening char and
 * short to int, but does not promote float to double (caller must do that).
 *
 * Odd sizes (3,5,6,7) come back beginning at the start of val->doub.
 * These are only possible if the caller specifies cb != 0, since all
 * the base types have "typical" sizes.  Note that cb < 0 means bit field.
 *
 * This routine is not designed for more than a double.  Unfortunately,
 * it gets called as a side-effect even when processing, say, a structure
 * field, so we can't even give a warning of truncation here.  However,
 * note that PsFDoOp() has independent provision for handling larger
 * values, so it's normally not a problem.
 */

export void ValFAdrTy (val, adrLong, ty, cbIn, fNeedLong, fWantAdr)
    register STUFFU	*val;			/* where to return data	*/
    long	adrLong;			/* adr of the value	*/
    register pTYR	ty;			/* type of the value	*/
    int		cbIn;				/* size; 0 => base size	*/
    FLAGT	fNeedLong;			/* need cast to long?	*/
    FLAGT	fWantAdr;			/* treat as adr anyway  */
{
    register int	cb;			/* bytes to get		*/
    ADRT	adrSrc = adrLong;		/* for copying value	*/
    ADRT	adrDest;
    int		bt     = ty->td.bt;		/* base type		*/
    int		tq1    = TqFTy (ty, 1);		/* top tq		*/
    FLAGT	fConst = ty->td.fConstant;

    *val = astuffZeros;				/* clear return data area */

    cb = CbFTy (ty, false);			/* default = base size */

    if (cbIn AND (cb >= 0))			/* size given, not bit field */
	cb = cbIn;

    if (cb > CBDOUBLE)				/* bt too long	*/
	cb = CBDOUBLE;				/* truncate	*/

    if (fConst AND (tq1 == tqNil) AND (bt == btDouble))
    {
	pDXTYR dxtyr = (pDXTYR) (ty + 1);	/* special use of XTYR */
	val->doub    = dxtyr->doub;		/* get double constant */
	goto Convert;
    }

    if ((!fWantAdr AND fConst) OR (tq1 == tqArray)) /* simple const, or array */
    {
	val->lng = adrLong;			/* adr is the value */
	goto Convert;
    }

/*
 * GET A REGISTER:
 *
 * Pc is the only register that is modified internal to the debugger for
 * a long time before writing it back to the child.  For this special case
 * we use vpc rather than read the value from the child.
 */

    if (ty->td.st == stReg)
    {
	REGT	regVal = (adrSrc == upc) ? vpc : GetReg (adrSrc);

	if (cb == CBCHAR)
	{
	    val->chars.chLoLo	= regVal;
	}
	else if (cb == CBSHORT)
	{
	    val->shorts.shortLo = regVal;
	}
	else
	{
#if (CBINT == CBSHORT)
/*
 * They want a long reg value but vpc is short; have no choice but to read it:
 */
	    val->shorts.shortHi = GetReg (adrSrc);
	    val->shorts.shortLo = GetReg (adrSrc + 1);
#else
	    val->lng = regVal;
#endif
	}


	goto Convert;

    } /* if */


/*
 * FIGURE WHERE TO SAVE the value (in debugger memory), according to its size:
 */

    adrDest = (cb == CBCHAR)  ?	(ADRT) & (val->chars.chLoLo)   :
	      (cb == CBSHORT) ?	(ADRT) & (val->shorts.shortLo) :
	      (cb == CBLONG)  ?	(ADRT) & (val->lng)	       :
				(ADRT) & (val->doub);	/* odd sizes too */

/*
 * GET A DEBUGGER SPECIAL:
 */

    if (ty->td.st == stSpc)
    {
	MoveBytes (adrDest, adrSrc, cb);	/* copy in debugger memory */
	goto Convert;
    }

    if (adrSrc == adrNil)		/* nil address; use zero */
	goto Convert;

/*
 * GET A DATA VALUE:
 */

    if (cb > 0)				/* normal case; just get the data */
    {
	GetBlock (adrSrc, spaceData, adrDest, cb);

    }					/* fall to sign extension */
    else
    {					/* bit field specifier */

	long	valTemp;
					/* avoid unsigned divide */
	cb = ((long) (ty->valTy + ty->td.width + (SZCHAR-1))) / SZCHAR;
	GetBlock (adrSrc, spaceData, & valTemp, cb);
	val->lng = Extract (valTemp, ty->valTy, ty->td.width);
	return;		/* always unsigned, don't extend, no need to cast */
    }

/*
 * DO CONVERSIONS:  sign extension, cast to long:
 *
 * This is a mess because sign extension is needed if a small, signed basetype
 * was used, OR if the caller gave a small size and the basetype was signed.
 */

Convert:

    if ((tq1 == tqNil) OR cbIn)		/* simple basetype or size given */
    {
	FLAGT	fSigned = ! (FUnsigned (bt));	/* basetype signed? */

	if (((! cbIn) AND (bt   == btChar))	/* really is signed char   */
	OR  (fSigned  AND (cbIn == CBCHAR)))	/* signed and size of char */
	{
	    val->lng = val->chars.chLoLo;	/* sign extend */
	}
	else if (((! cbIn) AND (bt   == btShort))
	     OR  (fSigned  AND (cbIn == CBSHORT)))
	{
	    val->lng = val->shorts.shortLo;	/* sign extend */
	}

#if (CBINT == CBSHORT)
	else if (((! cbIn) AND (bt   == btInt))
	     OR  (fSigned  AND (cbIn == CBINT)))
	{
	    val->lng = val->shorts.shortLo;	/* sign extend */
	}
#endif

	else if (fNeedLong)
	{
	    if (bt == btFloat)			/* shorten */
		val->lng = val->fl;

	    else if (bt == btDouble)		/* shorten */
		val->lng = val->doub;
	}
    }

} /* ValFAdrTy */


/***********************************************************************
 * P U T   V A L
 *
 * Given a value, address, and type, save the value at the place pointed
 * to by the address, if possible.  Caller must do type conversion if
 * needed; it only does size conversion.
 *
 * Odd sizes (3,5,6,7) are not supported, since the routine and its
 * caller only have type info, with no separate size info.
 *
 * This routine is not designed for more than a double.  Unfortunately,
 * it gets called as a side-effect even when processing, say, a structure
 * field, so we can't even give a warning of truncation here.  However,
 * note that PsFDoOp() has independent provision for handling larger
 * values, so it's usually not a problem.
 */

local void PutVal (val, adrLong, ty)
    STUFFU	val;				/* value to put		*/
    long	adrLong;			/* adr of the value	*/
    register pTYR	ty;			/* type of the value	*/
{
    ADRT	adrSrc;				/* for copying value	*/
    ADRT	adrDest	= adrLong;
    register int  cb	= CbFTy (ty, false);	/* size of the value	*/

    if (cb > CBDOUBLE)				/* bt too long	*/
	cb = CBDOUBLE;				/* truncate	*/

/*
 * FIGURE WHERE TO GET the value (in debugger memory), according to its size:
 */

    adrSrc = ((cb == CBCHAR)  ?	(ADRT) & (val.chars.chLoLo)   :
	      (cb == CBSHORT) ?	(ADRT) & (val.shorts.shortLo) :
	      (cb == CBLONG)  ?	(ADRT) & (val.lng)	      :
				(ADRT) & (val.doub));	/* odd sizes too */


/*
 * PUT A REGISTER:
 */

    if (ty->td.st == stReg)
    {



	if (cb == CBCHAR)
	    PutReg (adrDest, val.chars.chLoLo);
	else if (cb == CBSHORT)
	    PutReg (adrDest, val.shorts.shortLo);
	else
	{
#if (CBINT == CBSHORT)
	    PutReg (adrDest,	 val.shorts.shortHi);
	    PutReg (adrDest + 1, val.shorts.shortLo);
#else
	    PutReg (adrDest, val.lng);
#endif
	}



	return;

    } /* if */

/*
 * PUT A DEBUGGER SPECIAL:
 */

    if (ty->td.st == stSpc)			/* a special (debugger) value */
    {
	MoveBytes (adrDest, adrSrc, cb);	/* copy in debugger memory */
	return;
    }

/*
 * PUT A DATA VALUE:
 */

    if (cb > 0)
    {


	PutBlock (adrDest, spaceData, adrSrc, cb);
    }
    else					/* bit field specifier */
    {

	long	valTemp;
						/* avoid unsigned divide */
	cb = ((long) (ty->valTy + ty->td.width + (SZCHAR-1))) / SZCHAR;
	GetBlock (adrDest, spaceData, & valTemp, cb);
	SetBits	 (valTemp, ty->td.width, ty->valTy, val.lng);
	PutBlock (adrDest, spaceData, & valTemp, cb);

    }

} /* PutVal */


/***********************************************************************
 * P S	 F   D O   O P
 *
 * This HUGE routine actually does the top operator.  It uses up (pops)
 * an operator and one or two operands, and pushes a result operand.  It
 * returns psFail if something goes wrong (should never happen), else
 * psVar.  Tries to do everything according to C rules, with some
 * extensions allowed (like pointer and logical math on real numbers,
 * which are cast to longs).  All floating math is done with doubles only.
 *
 * Note that ValFAdrTy() already lengthens operands according to C rules
 * (EXCEPT float to double), but it can't handle arbitrarily long values.
 * This routine handles assigns larger than CBDOUBLE bytes, independent of
 * ValFAdrTy() and PutVal(), and refuses to do arithmetic on anything too
 * long for the Val routines.  It also knows how to squirrel away constant
 * doubles in the XTYR since they can't be put on the stack.
 *
 * Erases the type name after doing the operation, to avoid confusing
 * data displays.
 */

local PSE PsFDoOp()
{
    register TKE	tk;		/* current operator		*/

    TYR		aty1  [cTyMax];		/* holds type of left  operand	*/
    TYR		aty2  [cTyMax];		/* holds type of right operand	*/
    TYR		atyRes[cTyMax];		/* holds type of result		*/
    register pTYR  ty1   = aty1;	/* left-hand or only operand	*/
    register pTYR  ty2   = aty2;	/* right-hand operand if any	*/
    register pTYR  tyRes = atyRes;	/* larger of two operand types	*/

    long	adr1,	 adr2;		/* adrs	of operands, as longs	*/
    STUFFU	stuff1,	 stuff2;	/* raw values of operands	*/
    register long   val1,  val2;	/* vals	of operands, as longs	*/
    register ulong  valu1,  valu2;	/* vals	of operands, as unsigns	*/
    double	vald1,	 vald2;		/* vals	of operands, as doubles	*/
    int		bt1,	 bt2;		/* base types of operands	*/
    int		tq11,	 tq21;		/* top type qualifiers		*/
    int		cb1,	 cb2;		/* sizes of operands		*/
    FLAGT	fFloat1, fFloat2;	/* floating operands?		*/

    FLAGT	fFloating;		/* floating operation needed?	*/
    FLAGT	fUnsigned;		/* unsigned operation needed?	*/
    FLAGT	fForceNoFloat;		/* non-floating-point result?	*/
    FLAGT	fForceInt;		/* simple int result?		*/

    FLAGT	fDidIt;			/* carried out operation?	*/
    FLAGT	fEraseName = true;	/* erase type name?		*/

#define cbAssMax 200			/* most bytes we can assign	*/
    char	buf [cbAssMax];		/* used to do big assigns	*/

    CopyTy(tyRes,vtyZeros);		/* zero out local var		*/

/*
 * GET TOP OPERATOR:
 */

    if (viopMac <= 0)				/* no more operators */
	return (psFail);

    tk = PopOp();				/* get top operator */

/*
 * GET OPERAND 2 (RIGHT HAND SIDE):
 */

    if (! FPopVar (& adr2, ty2))
	return (psFail);

    /* use base size; don't cast to long */
    ValFAdrTy (& stuff2, adr2, ty2, 0, false, false);

    bt2     = ty2->td.bt;
    tq21    = TqFTy (ty2, 1);
    /* cb2  = not worth setting */
    fFloat2 = (tq21 == tqNil) AND ((bt2 == btFloat) OR (bt2 == btDouble));

    if (fFloat2 AND (bt2 == btFloat))		/* need to promote value */
	stuff2.doub = stuff2.fl;

    val2 = fFloat2 ? (long) stuff2.doub : stuff2.lng;	/* for use as a long */

/*
 * TRY UNARY OPERATORS:
 */

    fDidIt  = true;

    switch (tk)
    {
	default:				/* NOT UNARY */
		fDidIt = false;
		break;

	case tkDeref:				/* DEREFERENCE ("*") */
		if (ty2->td.fConstant)
		{	val2 = GetWord(adr2);
		}

		if (tq21 == tqArray)		/* pointer to self  */
		    val2 =  adr2;		/* ("*x" == "x[0]") */

		AdjTd (ty2, tqNil);		/* remove tq, if any */

		/*
		 * Force ValFAdrTy() to do a (non-constant) lookup later;
		 * note that val2 == adr2 (or cast of a double) if it
		 * already was constant.
		 */

		ty2->td.fConstant	= false;
		ty2->td.st		= stValue;

		PushVar (val2, ty2);		/* allow real as a long */
		break;

	case tkRef:				/* REFERENCE ("&") */
		if (ty2->td.fConstant)
		    ExprError ("Can't take the address of a constant");

		if (ty2->td.st == stReg)
		    ExprError ("Can't take the address of a register");

		if (tq21 == tqArray)		/* "&array" == "array" */
		    fEraseName = false;		/* no need to do more  */
		else
		{
		    AdjTd (ty2, tqPtr, isymNil);	/* add pointer tq */
		    ty2->td.fConstant = true;	/* means val == adr */
		    ty2->td.st	      = stValue;
		}
		PushVar (adr2, ty2);		/* push adr, not val */
		break;

	case tkUMinus:				/* UNARY MINUS ("-") */
		ty2->td.fConstant = true;
		ty2->td.st	  = stValue;

		if (fFloat2)			/* save val in special place */
		{
		    pDXTYR dxtyr = (pDXTYR) (ty2 + 1);
		    dxtyr->doub  = -stuff2.doub;

		    ty2->td.bt  = btDouble;	/* so we know where value is */
		    ty2->td.tq1 = tqNil;	/* double result is simple   */
		}

		PushVar (-val2, ty2);		/* junk val if it's floating */
		break;

	case tkBang:				/* LOGICAL NOT ("!") */
		if (fFloat2)
		    PushVar (lengthen (stuff2.doub == 0.0), vtyCnInt);
		else
		    PushVar (lengthen (val2 == 0),	    vtyCnInt);

		break;

	case tkTilda:				/* BIT INVERT ("~") */
		ty2->td.fConstant = true;
		ty2->td.st	  = stValue;

		if (fFloat2)			/* allow real as an int */
		    ty2->td.bt = btInt;

		PushVar (~val2, ty2);
		break;

	case tkPlusPlus:			/* PRE-INCREMENT */
		ExprError ("Prefix \"++\" not supported");

	case tkMinusMinus:			/* PRE-DECREMENT */
		ExprError ("Prefix \"--\" not supported");

	case tkSizeof:				/* SIZE OF (sizeof)    */
		PushVar (CbFTy (ty2, true), vtyCnInt);	/* whole array */
		break;

#ifdef KASSERT
	case tkInside:				/* INSIDE PROC? ($in)	  */
		{   int	ipd = IpdFAdr ((ADRT) adr2);	/* proc inside of */

		    if (ipd == ipdNil)
			val2 = 0;		/* not in known proc */
		    else
			val2 = (vpc >= vrgPd[ipd].adrStart)
			   AND (vpc <  vrgPd[ipd].adrEnd);
		}
		PushVar (val2, vtyCnInt);	/* a true/false val */
		break;
#endif

    } /* switch */

    if (fDidIt)					/* did something */
	goto EraseName;				/* returns psVar */

/*
 * GET OPERAND 1 (LEFT HAND SIDE):
 */

    if (! FPopVar (& adr1, ty1))
	return (psFail);

    /* use base size; don't cast to long */
    ValFAdrTy (& stuff1, adr1, ty1, 0, false, false);

    bt1     = ty1->td.bt;
    tq11    = TqFTy (ty1, 1);
    cb1	    = CbFTy (ty1, false);
    fFloat1 = (tq11 == tqNil) AND ((bt1 == btFloat) OR (bt1 == btDouble));

    if (fFloat1 AND (bt1 == btFloat))		/* need to promote value */
	stuff1.doub = stuff1.fl;

    val1 = fFloat1 ? (long) stuff1.doub : stuff1.lng;	/* for use as a long */

/*
 * TRY PRIMARY-EXPRESSION OPERATORS (all binary):
 */

    fDidIt = true;

    switch (tk)
    {
	default:				/* NOT PRIMARY-EXPR */
		fDidIt = false;
		break;

	case tkIndex:				/* ARRAY [INDEX]   */
		if (tq11 == tqArray)		/* pointer to self */
		    val1 =  adr1;
						/* allow real as a long */
		{
		    long cbArray;		/* size of sub-array */
		    long lbArray, hbArray;	/* array bounds	     */

		    DimFTy (ty1,  1, & cbArray, & lbArray, & hbArray);
		    AdjTd (ty1, tqNil);		/* remove ptr / array */
		    ty1->td.fConstant	= false;
		    ty1->td.st		= stValue;
		    PushVar (val1 + ((val2 - lbArray) * cbArray), ty1);
		}


		break;

	case tkPtr:				/* PTR -> FIELD	    */
		adr1 = stuff1.lng;
		AdjTd (ty1, tqNil);
		ty1->td.fConstant	= false;
		ty1->td.st		= stValue;
						/* and fall through */

	case tkDot:				/* STRUCT.FIELD	    */
		val1 = adr1;			/* use adr directly */
		/* ty1 is struct; ty2 is field */
		PushVar (AdrFField (val1, ty1, ty2), ty2);
		fEraseName = false;		/* field name is valid */
		break;

    } /* switch */

    if (fDidIt)					/* did something */
	goto EraseName;				/* returns psVar */

/*
 * HANDLE SPECIAL CASE OPERATOR "ASSIGN WITH", e.g. "+=" or ">>=":
 *
 * Convert an "assign with" operator into an ordinary binary operator followed
 * by assignment:  Push tkAssign operator on operator stack and target variable
 * on operand stack.  Reassign current token (tk) to the binary operator
 * associated with the assignment.  The binary op gets done when we fall
 * through to the next section of code.  The assign gets done the next time
 * this procedure gets executed.  Assumes that PsFDoOp() is called within a
 * loop subject to operator precedence conditions.
 */

    fDidIt = true;

    switch (tk)
    {
	default:		fDidIt = false;		break;
	case tkAssMult:		tk = tkMul;		break;
	case tkAssDiv:		tk = tkDiv;		break;
	case tkAssMod:		tk = tkModulo;		break;
	case tkAssPlus:		tk = tkPlus;		break;
	case tkAssMinus:	tk = tkMinus;		break;
	case tkAssRight:	tk = tkRShift;		break;
	case tkAssLeft:		tk = tkLShift;		break;
	case tkAssBAND:		tk = tkBitAnd;		break;
	case tkAssXOR:		tk = tkXOR;		break;
	case tkAssBOR:		tk = tkBitOr;		break;
    }

    if (fDidIt)
    {
	PushVar (adr1, ty1);			/* replicate the top variable */
	PushOp	(tkAssign);			/* push assign on stack	      */
    }

/*
 * CHECK THAT OP IS ASSIGN OR BOTH OPERANDS ARE ARITHMETIC:
 *
 * MaxFTyTy() determines if the types are arithmetic operands.  If so, it
 * copies into tyRes the ty of the larger of the two operands.  Otherwise the
 * operation is illegal (except tkAssign).  Note that tkDot and tkPtr were
 * already handled.
 */


/*
 * CONVERT (PROMOTE) OPERANDS TO COMPATIBLE TYPES:
 *
 * Note that val1 and val2 are already long.
 */

    if (fFloating = (fFloat1 OR fFloat2))
    {
	vald1 = fFloat1 ? stuff1.doub : val1;	/* lengthen if needed */
	vald2 = fFloat2 ? stuff2.doub : val2;
    }
    if (fUnsigned = (FUnsigned (bt1) OR FUnsigned (bt2)))
    {
	valu1 = val1;				/* unsigned versions */
	valu2 = val2;
    }

/*
 * TRY BINARY OPERATORS (huge switch):
 *
 * The location of the result (vald1, valu1, or val1) is determined by
 * fFloating and fUnsigned (already set and not changed in the switch)
 * and by fForceNoFloat and fForceInt (false now and set in the switch).
 *
 * Both fFloating and fUnsigned may be set if one operand is of each type;
 * the first takes precedence when floats are allowable.
 */

    fForceNoFloat = false;
    fForceInt	  = false;
    fDidIt	  = true;

    switch (tk)				/* cases are in C precedence order */
    {
	default:
		fDidIt = false;
		break;

	case tkMul:
		if	(fFloating)	vald1 *= vald2;
		else if	(fUnsigned)	valu1 *= valu2;
		else			val1  *= val2;
		break;

	case tkDiv:
		if	(fFloating)	vald1 /= vald2;
		else if (fUnsigned)	valu1 /= valu2;
		else			val1  /= val2;
		break;

	case tkModulo:
		fForceNoFloat = true;	/* allow real as a long */

		if	(fUnsigned)	valu1 %= valu2;
		else			val1  %= val2;
		break;

	case tkPlus:
		if	(fFloating)	vald1 += vald2;
		else if	(fUnsigned)	valu1 += valu2;
		else			val1  += val2;
		break;

	case tkMinus:
		if	(fFloating)	vald1 -= vald2;
		else if (fUnsigned)	valu1 -= valu2;
		else			val1  -= val2;
		break;

	case tkRShift:
		fForceNoFloat = true;	/* allow real as a long  */
					/* val2 is always signed */
		if	(fUnsigned)	valu1 >>= val2;
		else			val1  >>= val2;
		break;

	case tkLShift:
		fForceNoFloat = true;	/* allow real as a long  */
					/* val2 is always signed */
		if	(fUnsigned)	valu1 <<= val2;
		else			val1  <<= val2;
		break;

	case tkLT:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 < vald2);
		else if (fUnsigned)	val1 = (valu1 < valu2);
		else			val1 = (val1  < val2);
		break;

	case tkGT:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 > vald2);
		else if (fUnsigned)	val1 = (valu1 > valu2);
		else			val1 = (val1  > val2);
		break;

	case tkLE:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 <= vald2);
		else if (fUnsigned)	val1 = (valu1 <= valu2);
		else			val1 = (val1  <= val2);
		break;

	case tkGE:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 >= vald2);
		else if (fUnsigned)	val1 = (valu1 >= valu2);
		else			val1 = (val1  >= val2);
		break;

	case tkEqual:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 == vald2);
		else			val1 = (val1  == val2);
					/* suffices for unsigned too */
		break;

	case tkNotEqual:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 != vald2);
		else			val1 = (val1  != val2);
					/* suffices for unsigned too */
		break;

	case tkBitAnd:
		fForceNoFloat = true;	/* allow real as a long */

		if	(fUnsigned)	valu1 &= valu2;
		else			val1  &= val2;
		break;

	case tkXOR:
		fForceNoFloat = true;	/* allow real as a long */

		if	(fUnsigned)	valu1 ^= valu2;
		else			val1  ^= val2;
		break;

	case tkBitOr:
		fForceNoFloat = true;	/* allow real as a long */

		if	(fUnsigned)	valu1 |= valu2;
		else			val1  |= val2;
		break;

	case tkLAND:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 && vald2);
		else			val1 = (val1  && val2);
					/* suffices for unsigned too */
		break;

	case tkLOR:
		fForceInt = true;	/* result is always int */

		if	(fFloating)	val1 = (vald1 || vald2);
		else			val1 = (val1  || val2);
					/* suffices for unsigned too */
		break;

     /* case tkCondition:	(unsupported)		break; */

    } /* switch */

/*
 * BINARY OP WORKED; SAVE RESULTS:
 *
 * First change the type of the result if necessary.
 */

    if (fDidIt)
    {
	if (fFloating AND fForceNoFloat)	/* result can't be float */
	{
	    fFloating	 = false;
	    tyRes->td.bt = fUnsigned ? btUInt : btInt;	/* make UInt or Int */
	}

	if (fForceInt)				/* result must be int */
	{
	    fFloating	 = fUnsigned = false;
	    tyRes->td.bt = btInt;
	}

	if (fFloating)				/* save val in special place */
	{
	    pDXTYR dxtyr = (pDXTYR) (tyRes + 1);
	    dxtyr->doub  = vald1;

	    tyRes->td.bt  = btDouble;		/* so we know where value is */
	    tyRes->td.tq1 = tqNil;		/* double result is simple   */
	}					/* (e.g. (pointer + 1.6))    */

	tyRes->td.fConstant = true;
	tyRes->td.st	    = stValue;

	PushVar ((fUnsigned ? valu1 : val1), tyRes);	/* save result + type */
	goto EraseName;					/* returns psVar      */
    }

    if (tk != tkAssign)
	ExprError ("Unknown operator (%d)", tk);

/*
 * HANDLE BINARY ASSIGN OPERATOR OF A SMALL VALUE:
 *
 * Doesn't use tyRes set earlier; just uses the two types (ty1 and ty2)
 * and related values.  Note that unfortunately stuff2 may already have
 * been promoted from float to double.
 */

    if (cb1 <= CBDOUBLE)
    {
	if (fFloat2)
	    ty2->td.bt = btDouble;		/* be sure it's set right */

	ConvType (& stuff2, ty1,  ty2);		/* do type conversion */
	PutVal	 (  stuff2, adr1, ty1);		/* do the assignment  */
    }

/*
 * HANDLE BINARY ASSIGN OPERATOR OF A LARGE VALUE:
 *
 * This is done by copying bytes using a buffer.
 * Always copies the number of bytes in the destination object.
 */

    else
    {
	if (cb1 > cbAssMax)			/* it's too big */
	    Panic ("Attempt to assign into %d byte object; limit is %d bytes",
		   cb1, cbAssMax);

	GetBlock ((ADRT) adr2, spaceData, (ADRT) buf, cb1);
	PutBlock ((ADRT) adr1, spaceData, (ADRT) buf, cb1);

	cb2 = CbFTy (ty2, false);		/* element size only */

	if (cb1 != cb2)
	    printf (
	    "Assigning to %d byte object from %d byte object; moved %d bytes\n",
		cb1, cb2, cb1);
    }

    PushVar (adr1, ty1);			/* save the result	*/
    fEraseName = false;				/* LHS name still valid	*/

EraseName:

    if (fEraseName)			/* toss name -- no longer correct */
    {
	FPopVar (& adr1, ty1);

	ty1->sbVar = -1;

	PushVar (adr1, ty1);			/* save the result */
    }

    return (psVar);

} /* PsFDoOp */


/***********************************************************************
 * P S	 F   O P E R A T O R
 *
 * This routine implements the precedence rules.  It's called with an
 * operator and the current parser state.  It stacks operands and
 * operators as they come along, and calls PsFDoOp() to pop them and
 * carry out operations as required (as lower-precedence operators are
 * encountered).
 *
 * It returns the new parser state as psOp (after pushing a unary or
 * binary operator), psVar (after completing an atomic unit such as
 * inside "()"), or psParam (after completing a parameter).  It never
 * returns psFail, but instead handles its own errors.
 *
 * For paired tokens (such as "[]"), the left token is treated as an
 * operator of high precedence, but with the special exception that it
 * is not "evaluated" until the right token is encountered.
 */

local PSE PsFOperator (tk, ps)
    register TKE	tk;		/* the operator to do */
    PSE		ps;			/* the current state  */
{
    int		prec = PrecFTk (tk);	/* operator's precedence    */
    int		precTop;		/* precedence of top op	    */
    register TKE	tkStop;		/* don't pop ops past it    */
    register TKE	tkTop;		/* top stacked operator	    */

    if (prec == 0)
	Panic (vsbErrSinSD, "Invalid token", "PsFOperator", tk);

/*
 * HAVE AN OPERATOR FOLLOWING AN OPERATOR (e.g. a UNARY):
 *
 * Note that "(" is treated as a unary operator, e.g. it can follow any
 * other operator.  Also, Pascal deref is handled later (special case).
 */

    if (ps == psOp)
    {
	switch (tk)
	{
	    default:		ExprError (((TOPOP == tkBOE) OR (tk == tkEOE)) ?
					 sbErrExpr : "Two operators in a row");

	    /* these map to something else */
	    case tkAmper:	PushOp (tkRef);		break;
	    case tkStar:	PushOp (tkDeref);	break;
	    case tkPlus:	/* ignore it */		break;
	    case tkMinus:	PushOp (tkUMinus);	break;
	    case tkComma:	PushOp (tkBOP);		break;

	    case tkBang:
	    case tkTilda:
	    case tkPlusPlus:				/* prefix form */
	    case tkMinusMinus:				/* prefix form */
	    case tkSizeof:
	    case tkInside:
	    case tkLP:		PushOp (tk);		break;
	}
	return (psOp);				/* just did an operator */
    }

/*
 * HAVE AN OPERATOR FOLLOWING AN OPERAND:
 */

    if (tk == tkPlusPlus)
	ExprError ("Postfix \"++\" not supported");

    if (tk == tkMinusMinus)
	ExprError ("Postfix \"--\" not supported");

/*
 * SET STOP TOKEN for "()", "(x,y,z)", "[]", and whole expressions:
 *
 * This is the matching token that should already be on the operator stack.
 * The special case of ",)" for parameter lists is handled later.
 */

    switch (tk)
    {
	default:	tkStop = tkNil;	break;
	case tkRP:	tkStop = tkLP;	break;
	case tkComma:	tkStop = tkBOP;	break;
	case tkRSB:	tkStop = tkLSB;	break;
	case tkEOE:	tkStop = tkBOE;	break;
    }

/*
 * PROCESS ALL HIGHER OR EQUAL PRECEDENCE OPERATORS on the stack:
 *
 * Returns if we close an atomic unit such as "()", or ",)" as a special case.
 * Treats operators inside atomic units as being at higher precedence, but
 * watches for end of expression with start of unit still on stack.
 */

    while (((precTop = PrecFTk (tkTop = TOPOP)) > prec)	/* higher one	    */
    OR     ((precTop == prec) AND FLeftAssoc (tkTop)))	/* or is left assoc */
    {
	if ( (tkTop == tkStop)			  /* hit stop op  */
	OR  ((tkTop == tkBOP) AND (tk == tkRP)))  /* special case */
	{
	    PopOp();				  /* eat stop operator */
	    return ((tkTop == tkBOP) ? psParam : psVar);
	}

	if ((tkTop == tkBOE)
	OR  (tkTop == tkBOP)
	OR  (tkTop == tkLP)
	OR  (tkTop == tkLSB))			/* start of atomic unit */
	{
	    if (tk == tkEOE)			/* but no more expression */
		ExprError (sbErrExpr);

	    break;				/* pretend tk is higher prec */
	}

	if (PsFDoOp() == psFail)		/* should never fail, but... */
	    ExprError (sbErrExpr);

    } /* while */

/*
 * PUSH THE CURRENT OPERATOR:
 */

    if (tk == tkDerefP)			/* Pascal dereference ("operand^") */
    {
	PushOp (tkDeref);		/* treat as a simple dereference */
	return (psVar);			/* fake to look like did operand */
    }

    if (tk == tkLSB)			/* first tkIndex, then tkLSB */
	PushOp (tkIndex);

    PushOp (tk);
    return (psOp);			/* just did an op */

} /* PsFOperator */


/***********************************************************************
 * T K	 F   O P E R A N D
 *
 * Look at the current token (vtk) and figure out what it is.  If it's
 * not recognized, don't touch the command line, and return tkNil or
 * tkStr (if appropriate).  Otherwise, push info on the operand stack
 * and return tkAdr or tkNumber, with vtk set to the last token used
 * (NOT the next token).  Does not affect the parser state or operator
 * stack.  If it only halfway recognizes something it errors out.
 * 
 * This is one big switch.  Some of the cases which set adr actually
 * set it to a constant value (and ty to a constant type).
 */

local TKE TkFOperand (ps)
    PSE		ps;			/* current parser state	*/
{
    ADRT	adr;			/* address of interest	*/
    TYR		ty [cTyMax];		/* type of result	*/
    long	val;			/* value of interest	*/
    int		ipd;			/* proc of interest	*/
    FLAGT	fLocalVar = false;	/* is a local variable?	*/

    switch (vtk)
    {
	default: return (tkNil);			/* not of interest */

/*
 * PURE NUMBER CASES (all return tkNumber):
 */

	case tkHex:
	case tkDecimal:
	case tkOctal:
		FValFTok (& val);
	        PushVar (val, vtyCnInt);
		return (tkNumber);

#ifdef KFLPT
	case tkFloat:
	    {
		TYR	ty [cTyMax];			/* holds result type */
		pDXTYR	dxtyr = (pDXTYR) (ty + 1);	/* to access XTYR    */

		CopyTy (ty, vtyCnDouble);		/* set up type info   */
		dxtyr->doub = atof (vsbTok);		/* get and save value */
		PushVar (0L, ty);			/* nil adr for double */
		return (tkNumber);
	    }
#endif

/*
 * RANDOM CASES (all return tkAdr):
 */

	case tkDot:					/* "current" var */
		adr = vdot;
		CopyTy (ty, vtyDot);
		break;

	case tkColon:					/* want global var */
		ExprTkNext();
		if (vtk != tkStr)
		    ExprError ("Misformed global name");

		if ((adr = AdrFGlobal (vsbTok, ty)) == adrNil)
		    ExprError ("Unknown global");

		break;

	case tkCharConstant:				/* such as 'a'	  */
		adr = vsbTok[0];			/* actually value */
		CopyTy (ty, vtyCnChar);
		break;

	case tkStrConstant:				/* such as "abc" */
		printf("Sorry, string constants do not work\n");
		break;

/*
 * STRING CASE (FILE, PROC, OR VAR NAME) (return tkAdr or tkStr):
 *
 * Check first for struct->field or struct.field.
 */

	case tkStr:
		if ((TOPOP == tkDot) OR (TOPOP == tkPtr))
		{
		    /* just save field name; rest is processed later */
		    adr = 0L;				/* starts zero */
		    CopyTy (ty, vtyZeros);

		    ty[0].sbVar = SbSafe (vsbTok);	/* save the name */

		    break;
		}

/*
 * REGISTER OR SPECIAL VAR:
 */

		if (vsbTok[0] == '$')
		{
		    AdrFSpecial (vsbTok, ty, & adr);
		    break;
		}

/*
 * LOCAL OR GLOBAL VARIABLE:
 *
 * If it's a local but not on the stack, just note that for below, in case
 * it's not anything else either.  Note that fLocalVar implies vipd is valid.
 */

		if (vipd != ipdNil)			/* have current proc */
		{
		    if ((adr = AdrFLocal (vipd, -1, vsbTok, ty)) != adrNil)
			break;				/* have type AND adr */

		    fLocalVar = (ty->td.st != stNil);	/* have type only?   */
		}

		if ((adr = AdrFGlobal (vsbTok, ty)) != adrNil)
		    break;

/*
 * PROCEDURE NAME:
 */

		if ((ipd = IpdFName (vsbTok)) != ipdNil)
		{
		    if (TkPeek() == tkDot)		/* "proc." */
		    {
			ADRT	fp;			/* register values */
			long	depth = -1;		/* stack depth	   */

			FpFIpd (& fp, ipd, -1);	/* look up proc */

			if (fp == 0)
			    ExprError ("Procedure not active",
				vrgPd[ipd].sbProc);

			ExprTkNext();			/* set to "."	  */
			ExprTkNext();			/* set to unknown */

			if (FValFTok (& depth))		/* "proc.number" */
			{
			    ExprTkNext();
			    if (vtk != tkDot)		/* set to "." */
				ExprError ("Need a \".\" after the number");

			    ExprTkNext();		/* set to unknown */
			}

			if (vtk != tkStr)		/* "proc[.num].???" */
			    ExprError ("Invalid local name");

			if ((adr = AdrFLocal (ipd, depth, vsbTok, ty))
			== adrNil)
			    ExprError ("Unknown local");

			break;				/* "proc[.num].local" */
		    }

		    if (vtkPeek == tkHash)		/* "proc#" */
		    {
			int	ifd;
			long	iln;

			ExprTkNext();			/* set to "#"	  */
			ExprTkNext();			/* set to unknown */

			if (! FValFTok (& iln))
			   ExprError ("Need a line number after the \"#\"");

			ifd = IfdFIpd (ipd);
			adr = AdrFIfdLn (ifd, iln);	/* get line adr */
			CopyTy (ty, vtyCnInt);
							/* save the name */
		        ty[0].sbVar = SbSafe (vrgPd[ipd].sbProc);
			break;				/* "proc#line" */
		    }
		    else				/* "proc???" */
		    {
			adr = vrgPd[ipd].adrStart;	/* use adr of proc */
			CopyTy (ty, vtyCnInt);
		        ty[0].sbVar = SbSafe (vsbTok);	/* save the name */
			break;
		    }
		} /* if */

/*
 * UNDEBUGGABLE GLOBAL:
 */

		if ((adr = AdrFLabel (vsbTok)) != adrNil)
		{
		    CopyTy (ty, vtyCnInt);		/* use adr */
		    break;
		}

/*
 * UNKNOWN STRING:
 */

		if ((fLocalVar)		/* saw local type info */
		AND (ps == psOp))	/* expected an operand */
		{
		    ExprError ("Local is not active");
		}
		return (tkStr);		/* a string we can't handle */

    } /* switch */

/*
 * RETURN VALUES:
 */

    PushVar (lengthen (adr), ty);	/* adr and ty already set up */
    return  (tkAdr);

} /* TkFOperand */


/***********************************************************************
 * T K	 F   E X P R
 *
 * Evaluate an expression (if any) from the current position on the
 * command line (vtk) and return the net token type:  tkNil (no tokens,
 * or command line doesn't look like start of expr), tkNumber (pure
 * number), or tkAdr (expr result).  Once committed to doing an
 * expression, any error is fatal; if it returns tkNil, the command line
 * is not altered.
 *
 * Usually leaves the current token set to the NEXT one to process after
 * the expr, e.g. at tkNil, at something that doesn't look like an operand
 * or operator, or at a tkStr that might be a command after a previous
 * operand.  In the special case of a parameter list, leaves it set to the
 * current token (a comma or RP).
 *
 * ps (parser state) tells what we just parsed (operator, variable, or end
 * of parameter as a special case).  vsbExprSave accumulates the tokens
 * parsed so far, for error reporting.
 *
 * Since this routine is re-entrant (TkFOperand() can call DoProc()), it
 * is careful to remember the initial expr stack settings, and to not
 * initialize vsbExprSave except on the first call.
 */

export TKE TkFExpr (padr, ty)
    long	*padr;			/* where to return adr	*/
    pTYR	ty;			/* where to return type */
{
    register PSE	ps;		/* current parser state */
    register TKE	tkOper;		/* previous token type	*/

    int		viopSave   = viopMac;	/* note initial values */
    int		vivarSave  = vivarMac;

    if (FAtSep (vtk))			/* no next token */
    {
	*padr = 0;
	return (tkNil);
    }

/*
 * SET UP TO DO AN EXPRESSION (check for an operand or operator):
 *
 * Treats a pure number (not followed by an operator) as a special case.
 * An initial, unrecognized string (tkStr) is not an error here.
 * Note that "." (tkDot) looks like an operand here, not an operator,
 * since it checks for an operand first.  Also, tkBOE has to be pushed
 * BEFORE anyone (like TkFOperand()) uses the TOPOP macro, even though
 * we have to pop it for tkNumber.
 */

    vsbExprSave[0] = chNull;			/* initialize saved expr   */

    PushOp (tkBOE);				/* push beginning-of-expr */

    if (((tkOper = TkFOperand (psOp)) != tkNil)
    AND ( tkOper != tkStr))			/* OPERAND; already pushed */
    {
	ps = psVar;
	ExprTkNext();				/* see what's next    */

	if ((tkOper == tkNumber)		/* it's a PURE NUMBER	*/
	AND ! PrecFTk (vtk))			/* no operator follows	*/
	{
	    PopOp();				/* eat the BOE	  */
	    FPopVar (padr, ty);			/* eat the number */
	    return  (tkNumber);
	}
	/* else start with vtk == next token (unknown) and ps == psVar */
    }
    else					/* OPERATOR */
    {
	if (! PrecFTk (vtk))			/* oops, not operator either! */
	{
	    viopMac  = viopSave;		/* clean up the stacks */
	    vivarMac = vivarSave;
	    return (tkNil);
	}
	ps = psOp;

	/* else start with vtk == first token (operator) and ps == psOp */
    }

/*
 * LOOP THROUGH EXPRESSION:
 *
 * Each time into the loop vtk == next token (unknown).
 * Dot (current loc) is treated like an operator if we just did a variable.
 * Note that PsFOperator() handles its own errors, so we don't check here.
 * Also, an unrecognized string IS an error here.
 */

    while (vtk != tkNil && vtk != tkComma)	/* have a token to look at */
    {
	if (PrecFTk (vtk)			/* LOOKS LIKE OPERATOR	*/
	AND ((vtk != tkDot) OR (ps == psVar)))	/* and not vDot either	*/
	{
	    if ((ps = PsFOperator (vtk, ps)) == psParam)
		break;				/* finished a parameter */
	}
	else					/* LOOKS LIKE OPERAND	*/
	{
	    if ((tkOper = TkFOperand (ps)) == tkNil)  /* not operand either   */
		break;				      /* fall out, maybe done */

	    if (tkOper == tkStr)		/* unrecognized string	*/
	    {
		if (ps == psVar)		/* last was operand   */
		    break;			/* might be a command */

		ExprError ("Unknown name");	/* wanted an operand */
	    }
						/* here, have an operand */
	    if (ps == psVar)			/* last was operand too	 */
		ExprError ("Two operands in a row");

	    ps = psVar;
	}
	ExprTkNext();

    } /* while */

/*
 * SEEM TO BE DONE WITH EXPRESSION:
 */

    PsFOperator (tkEOE, ps);			/* returns if it worked */
    FPopVar (padr, ty);				/* get final adr and ty */

    if ((viopMac  != viopSave)
    OR  (vivarMac != vivarSave))		/* stacks got munched */
    {
	ExprError (sbErrExpr);
    }
    return  (tkAdr);

} /* TkFExpr */


/***********************************************************************
 * G E T   E X P R
 *
 * Parse and execute an expression and return its type and value (cast
 * to a long if was a float or double!).
 *
 * Ty is declared here for callers who don't want to be bothered with
 * allocating one themselves.  If they want this info for more than a
 * statement or two, they MUST copy it someplace else!
 *
 * The way tk is used, it could be a simple flag that just says whether
 * or not to start off with a TkNext().
 */

export long GetExpr (pty, tk)
    pTYR	*pty;			/* result expr type   */
    TKE		tk;			/* current token type */
{
    STUFFU	val = astuffZeros;	/* result of expression	  */
    long	adrLong;		/* adr of result (tossed) */
    static TYR	ty [cTyMax];		/* place to stash type	  */

    if (tk == tkNil)				/* no current token */
	TkNext();				/* so get one	    */

    if (TkFExpr (& adrLong, ty) == tkNil)	/* invalid or no expr */
	*pty = tyNil;				/* no useful return   */
    else
    {
	/* use base size; cast float to long */
	ValFAdrTy (& val, adrLong, ty, 0, true, false);
	*pty = ty;				/* note the type */
    }

    return (val.lng);

} /* GetExpr */
