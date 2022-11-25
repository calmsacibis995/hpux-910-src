/* @(#) $Revision: 10.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This file defines types, structures, etc. for token handling.
 */

/***********************************************************************
 * TOKEN TYPES:
 *
 * In ASCII order of chars, where possible, just to help organize things.
 *
 * These types are used both to represent types of current tokens and also
 * as types of operators and as types of expression results.
 *
 * When a token has a certain meaning (e.g. tkMul means tkStar), this is
 * done by equating one to the other.  Some of the basic types are never
 * used; only their "meaningful" forms are used.  In some cases a token
 * is set to one form and later referenced using the other form.
 *
 * When there are multiple meanings (a character is overloaded) then the
 * BINARY operator token has the same value as the basic symbol (e.g.
 * tkMul == tkStar) and the unary operator has its own token (e.g. tkDeref).
 * The context sensitive translation is done in expr.c.
 *
 * This WAS an enum, but went to defines for V7 compatibility.
 */

#define tkOther		(-1)
#define tkNil		  0

/*
 * SIMPLE ASCII CHARS:
 */
						/* skip NUL - BLANK */
#define tkBang		 1	/* ! */
#define tkDQuote	 2	/* " */
#define tkHash		 3	/* # */
#define tkDollar	 4	/* $ */
#define tkPercent	 5	/* % */
#define tkModulo tkPercent
#define tkAmper		 7	/* & */
#define tkBitAnd   tkAmper
#define tkSQuote	 8	/* ' */
#define tkLP		 9	/* ( */
#define tkRP		10	/* ) */
#define tkStar		11	/* * */
#define tkMul	    tkStar
#define tkPlus		12	/* + */
#define tkComma		13	/* , */
#define tkMinus		14	/* - */
#define tkDot		15	/* . */
#define tkSlash		16	/* / */
						/* skip 0 - 9 */
#define tkColon		20	/* : */
#define tkSemi		21	/* ; */
#define tkLAB		22	/* < */
#define tkLT	     tkLAB
#define tkAssign	23	/* = */
#define tkRAB		24	/* > */
#define tkGT	     tkRAB
#define tkQuest		25	/* ? */
#define tkAt		26	/* @ */
						/* skip A - Z */
#define tkLSB		30	/* [ */
#define tkBackSlash	31	/* \ */
#define tkRSB		32	/* ] */
#define tkUpArrow	33	/* ^ */
#define tkXOR	 tkUpArrow
#define tkUnderScore	34	/* _ */
						/* skip ` and a - z */
#define tkLCB		40	/* { */
#define tkBar		41	/* | */
#define tkBitOr	     tkBar
#define tkRCB		42	/* } */
#define tkTilda		43	/* ~ */

/*
 * OVERLOADED CHARS (unary tokens):
 */

#define tkRef		50	/* & */
#define tkDeref		51	/* * */
#define tkDerefP	52	/* ^ (for Pascal) */
#define tkUMinus	53	/* - */

/*
 * COMPOUND TOKENS (in K&R order):
 */

#define tkPtr		60	/* ->  */
#define tkPlusPlus	61	/* ++  */
#define tkMinusMinus	62	/* --  */
#define tkRShift	63	/* >>  */
#define tkLShift	64	/* <<  */
#define tkLE		65	/* <=  */
#define tkGE		66	/* >=  */
#define tkEqual		67	/* ==  */
#define tkNotEqual	68	/* !=  */
#define tkLAND		69	/* &&  */
#define tkLOR		70	/* ||  */
#define tkCondition	71	/* ?:  */ 	/* not implemented */
#define tkAssPlus	72	/* +=  */
#define tkAssMinus	73	/* -=  */
#define tkAssMult	74	/* *=  */
#define tkAssDiv	75	/* /=  */
#define tkAssMod	76	/* %=  */
#define tkAssRight	77	/* >>= */
#define tkAssLeft	78	/* <<= */
#define tkAssBAND	79	/* &=  */
#define tkAssXOR	80	/* ^=  */
#define tkAssBOR	81	/* |=  */

/*
 * SPECIAL OPERATORS:
 */

#define tkDiv		90	/* // division, not format */
#define tkIndex		91	/* index ("[]") operation  */
#define tkSizeof	92	/* the C sizeof operator   */
#define tkInside	93	/* inside of proc ("$in")  */

/*
 * EXPRESSIONS AND CONSTANTS:
 */

#define tkAdr		100	/* value is an address	   */
#define tkNumber	101	/* numeric result	   */
#define tkHex		102	/* hexadecimal number	   */
#define tkDecimal	103	/* decimal number	   */
#define tkOctal		104	/* octal number	   	   */
#define tkFloat		105	/* floating point number   */
#define tkStr		106	/* Hi			   */
#define tkStrConstant	107	/* "Hi\n"		   */
#define tkCharConstant	108	/* 'x'			   */

/*
 * OTHER TOKEN TYPES:
 */

#define tkBOE		110	/* beginning of expression */
#define tkEOE		111	/* end of expression	   */
#define tkBOP		112	/* beginning of parameter  */


/***********************************************************************
 * MISCELLANEOUS:
 */

typedef	int	 TKE, *pTKE;
#define	cbTokMax 100		/* max token size */

/*
 * This macro tells if we're at a command separator:
 * It should not be called with a function (e.g. TkNext()) but only
 * with a static object!
 */

#define	FAtSep(tk)	((tk == tkNil) OR (tk == tkSemi) OR (tk == tkRCB))
