/* @(#) $Revision: 66.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This large collection of routines is essentially used to build internal
 * type records (TYRs) from external symbol records (SYMRs) from the object
 * file.  Most of the knowledge about different symbol table structures is
 * concentrated here.
 */

#include <ctype.h>
#include "cdb.h"
#include "macdefs.h"

#define crockValue (-1)			/* nil name for type		*/
#define cbSbSafe   100			/* size of misc. string cache	*/

char	vsbSafe [cbSbSafe];		/* misc. string cache	*/
char	*vsbNextSafe = vsbSafe;		/* next available place */
local long CbFDntt();


/***********************************************************************
 * STANDARD TYPE RECORDS:
 *
 * These are a list of instances of each type, followed by pointers to
 * each type.  The boolean field is fConstant.
 *
 * Note that dummy extension records are NOT allocated after each TYR.
 * If someone copies a vty using CopyTy(), the second record is "junk".
 */

TYR	atyZeros;				/* defaults to all zeros */

TYR	atyCnChar  = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, btChar,  0};
TYR	atyCnInt   = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, btInt,   0};
TYR	atyLong	   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, btLong,  0};
#ifdef KFLPT
TYR	atyCnDouble= {crockValue,  stValue, true,  0, 0,0,0,0,0,0, btDouble,0};
#endif /* KFLPT */

export	pTYR	vtyZeros   = & atyZeros;
export	pTYR	vtyCnChar  = & atyCnChar;
export	pTYR	vtyCnInt   = & atyCnInt;
export	pTYR	vtyLong	   = & atyLong;
#ifdef KFLPT
export	pTYR	vtyCnDouble= & atyCnDouble;
#endif /* KFLPT */


/***********************************************************************
 * S B	 S A F E
 *
 * Stick a string in a temporarily safe place and return a pointer to it.
 */

export SBT SbSafe (sb)
    register char	*sb;		/* string to save */
{
    register char	*sbT = sb;	/* temp ptr	 */
    register int	cb;		/* bytes to save */
    SBT		sbRet;		/* ptr to return */

    while (*sbT != chNull)			/* find null */
	sbT++;

    cb = sbT - sb + 1;				/* bytes to save */

    if (cb > cbSbSafe)
	Panic (vsbErrSinSD, "String too large for buffer", "SbSafe", cb);

    if (vsbNextSafe - vsbSafe + cb >= cbSbSafe) /* passes end of buffer */
	vsbNextSafe = vsbSafe;			/* reuse the buffer	*/

    strncpy (vsbNextSafe, sb, cb);

    sbRet = bitHigh | (SBT) vsbNextSafe;	/* mark as char*, not iss */

    vsbNextSafe [cb - 1] = chNull;		/* add terminator */
    vsbNextSafe += cb;

    return (sbRet);

} /* SbSafe */


/***********************************************************************
 * S E T   W I D T H   (macro)
 *
 * If the type has not already picked up a width (the first one seen
 * while traipsing down the type chain), set the given width.  Since
 * the ty only provides room for 7 bits, we use a separate, global var
 * to hold the width.
 */

local	long	vwidthHp;

#define SetWidth(wide)    {if (vwidthHp == 0) vwidthHp = wide;}


/***********************************************************************
 * T Y	 F   I M M E D	 H P
 *
 * Given the parent symbol's kind and its immediate information, finish
 * filling out the ty record.  This means cramming HP symbol table info
 * into TYR format, which does not generally remember sizes of things.
 * visym must be set to the symbol containing type info.
 * 
 * Uses the SetWidth() macro to update vwidthHp, which the caller may use
 * to set ty->td.width as appropriate for bit fields.  Caller must init
 * vwidthHp == zero.
 */

local void TyFImmedHp (ty, di)
    pTYR	ty;			/* field type info to return	 */
    struct DNTTP_IMMEDIATE di;		/* this immediate value		 */
{
    register ulong	width = di.bitlength;	/* bit size of object	*/
    register int	bt;			/* equiv. base type	*/
    pXTYR	xty = (pXTYR) (ty + 1); /* quick pointer	*/

    switch (di.type)
    {

    default:		Panic (vsbErrSinSDD, "Invalid basetype", "TyFImmedHp",
					     di.type, visym);

    case T_UNDEFINED:	/* take a guess */
			if	(width <= SZCHAR)	bt = btChar;
			else if	(width <= SZSHORT)	bt = btShort;
			else				bt = btInt;
			break;

    case T_BOOLEAN:	if	(width == 1)		bt = btUInt;
			else if	(width == SZSHORT)	bt = btUShort;
			else if	(width == SZLONG)	bt = btULong;
			else	Panic (vsbErrSinSDD, "Invalid size for Boolean",
					"TyFImmedHp", width, visym);
			break;

    case T_CHAR:	bt = btChar;
			break;

    case T_INT:		/* btChar is the only way to remember size <= 8 */
			if	(width <= SZCHAR)	bt = btChar;
			else if	(width <= SZSHORT)	bt = btShort;
			else				bt = btLong;
			break;

    case T_UNS_INT:	/* symbol table deficient; C uses this for btUChar */
			if	(width <= SZCHAR)	bt = btUChar;
			else if	(width <= SZSHORT)	bt = btUShort;
			else				bt = btULong;
			break;

    case T_COMPLEX:	width /= 2;		/* and fall through	*/
			/* for COMPLEX, can only see first real in pair	*/

    case T_REAL:	if	(width == SZFLOAT)	bt = btFloat;
			else if	(width == SZDOUBLE)	bt = btDouble;
			else
				Panic (vsbErrSinSDD,
					"Invalid size for real number",
					 "TyFImmedHp", width, visym);
			break;

    case T_STRING200:
    case T_TEXT:
    case T_FLABEL:	bt	     = btNil;	/* special case */
			xty->st	     = stExtend;
			xty->btX     = di.type; /* hide HPSYMTAB type here */
			xty->isymRef = visym;	/* remember the symbol too */
			break;

    } /* switch */

    ty->td.bt = bt;
    SetWidth (di.bitlength);			/* use the original value */

} /* TyFImmedHp */


/***********************************************************************
 * T Y	 F   S C A N   H P
 *
 * Given a DNTTPOINTER (which may be immediate) and the parent symbol's
 * kind, add one level of information about a type to the ty record.
 * Only lower-level (unnamed) types are allowed here, with some
 * exceptions:  TYPEDEF, TAGDEF, and MEMENUM are legal too.
 *
 * Uses the SetWidth() macro to update vwidthHp, which the caller may
 * use to set ty->td.width as needed.  Caller must init vwidthHp == zero.
 *
 * This routine sets visym to a new value and does not reset it, for
 * efficiency reasons.  Callers must keep this in mind, and this routine
 * must not use current symbol values again after calling itself.
 */

local void TyFScanHp (ty, dp)
    register pTYR	ty;		/* field type info to return	*/
    DNTTPOINTER dp;			/* next symbol to handle	*/
{
    register pXTYR	xty = (pXTYR) (ty + 1); /* quick pointer	*/
    KINDTYPE	kind;			/* of this symbol		*/
    long	isym;			/* if it's an index, not immed	*/

    if (dp.word == DNTTNIL)			/* absent type info case */
	return;

    if (dp.dntti.immediate)			/* immediate case */
    {
	TyFImmedHp (ty, dp.dntti);
	return;
    }

/*
 * NON-IMMEDIATE CASE:
 */

    isym = dp.dnttp.index;
    SetSym (isym);
    kind = vsymCur->dblock.kind;

    switch (kind)
    {

    default:		Panic (vsbErrSinSDD, "Wrong symbol type", "TyFScanHp",
					     kind, visym);

    case K_TYPEDEF:	xty->isymRef = isym;
			TyFScanHp (ty, vsymCur->dtype.type);
			break;

    case K_TAGDEF:	xty->isymRef = isym;
			TyFScanHp (ty, vsymCur->dtag.type);
			break;

    case K_POINTER:	SetWidth  (    vsymCur->dptr.bitlength);
			TyFScanHp (ty, vsymCur->dptr.pointsto);
    			AdjTd	  (ty, tqPtr, isym);
			break;

    case K_ENUM:	ty->td.bt    = btEType;
			xty->st	     = stExtend;
			if (xty->isymRef == isymNil)
			    xty->isymRef = isym;
			xty->cb	     = vsymCur->denum.bitlength / SZCHAR;
			SetWidth (vsymCur->denum.bitlength);
			break;

    case K_MEMENUM:	ty->td.bt	 = btEMember;
			ty->valTy	 = vsymCur->dmember.value;
			ty->td.fConstant = true;
			xty->st		 = stExtend;
			xty->isymRef	 = isym;
			SetWidth (SZLONG);	/* size in SYMR */
			break;

    case K_SET:		ty->td.bt    = btNil;	/* special case */
			xty->st	     = stExtend;
			xty->btX     = TX_SET;	/* hide pseudo base type here */
			xty->isymRef = visym;	/* remember the symbol too    */
			SetWidth (vsymCur->dset.bitlength);
			break;

    case K_SUBRANGE:	SetWidth  (    vsymCur->dsubr.bitlength);
			TyFScanHp (ty, vsymCur->dsubr.subtype);
			break;

    case K_ARRAY:	if (vsymCur->darray.arrayisbytes) {
			    SetWidth(SZCHAR * vsymCur->darray.arraylength);
			}
			else {
			    SetWidth(vsymCur->darray.arraylength);
			}
			TyFScanHp (ty, vsymCur->darray.elemtype);
			AdjTd (ty, tqArray, isym);
			break;

    case K_STRUCT:	ty->td.bt    = btStruct;
			xty->st	     = stExtend;
			if (xty->isymRef == isymNil)
			    xty->isymRef = isym;
			xty->cb	     = vsymCur->dstruct.bitlength / SZCHAR;
			SetWidth (vsymCur->dstruct.bitlength);
			break;

    case K_UNION:	ty->td.bt    = btUnion;
			xty->st	     = stExtend;
			if (xty->isymRef == isymNil)
			    xty->isymRef = isym;
			xty->cb	     = vsymCur->dunion.bitlength / SZCHAR;
			SetWidth (vsymCur->dunion.bitlength);
			break;

    case K_FILE:	SetWidth  (    vsymCur->dfile.bitlength);
			TyFScanHp (ty, vsymCur->dfile.elemtype);
			break;

    case K_FUNCTYPE:	SetWidth  (    vsymCur->dfunctype.bitlength);
			TyFScanHp (ty, vsymCur->dfunctype.retval);
    			AdjTd	  (ty, tqFunc, isym);
			break;
    } /* switch */

} /* TyFScanHp */


/***********************************************************************
 * T Y	 F   H P
 *
 * Given a symbol index for a top-level (named) symbol, analyze the
 * external type information (from the object file) and force it into
 * the internal TYR format.  Most types require analyzing more than one
 * DNTT entry (e.g., following a chain) to figure out the type.
 *
 * Since ty->td.width is returned zero, doesn't bother to init vwidthHp.
 *
 * Note that fields are treated as a special case (not handled here).
 *
 * This routine sets visym to a new value and then uses it.  It does not
 * reset it, for efficiency reasons.  It also does not use current symbol
 * values after calling TyFScanHp(), which changes visym again.
 */

export void TyFHp (ty, isym)
    register pTYR	ty;		/* field type info to return	*/
    long	isym;			/* of symbol to handle		*/
{
    pXTYR	xty = (pXTYR) (ty + 1); /* quick pointer	*/
    KINDTYPE	kind;			/* kind of this isym	*/

    ty[0] = ty[1] = *vtyZeros;			/* clear out the type info */
    ty->sbVar = (SBT) NameFCurSym() | bitHigh;	/* remember the name	   */
    xty->isymRef = isymNil;

    SetSym (isym);
    kind = vsymCur->dtype.kind;


    switch (kind)
    {

    default:		Panic (vsbErrSinSDD, "Wrong symbol type", "TyFHp",
					     kind, visym);

    case K_FUNCTION:	TyFScanHp (ty, vsymCur->dfunc.retval);	break;
    case K_ENTRY:	TyFScanHp (ty, vsymCur->dentry.retval);	break;

    case K_FPARAM:	if (vsymCur->dfparam.regparam)
			    kind = stReg;		/* so st == stReg */
			TyFScanHp (ty, vsymCur->dfparam.type);
			break;

    case K_SVAR:	TyFScanHp (ty, vsymCur->dsvar.type);	break;

    case K_DVAR:	if (vsymCur->ddvar.regvar)
			    kind = stReg;		/* so st == stReg */
			TyFScanHp (ty, vsymCur->ddvar.type);	
			break;

    case K_CONST:	if ((vsymCur->dconst.locdesc == LOC_IMMED)
			OR  (vsymCur->dconst.locdesc == LOC_VT))
			    ty->td.fConstant = true;
			TyFScanHp (ty, vsymCur->dconst.type);	break;

    case K_TYPEDEF:	TyFScanHp (ty, vsymCur->dtype.type);	break;
    case K_TAGDEF:	TyFScanHp (ty, vsymCur->dtag.type);	break;

    case K_MEMENUM:	ty->td.bt	 = btEMember;
			ty->valTy	 = vsymCur->dmember.value;
			ty->td.fConstant = true;
			xty->st		 = stExtend;
			xty->isymRef	 = isym;	/* remember where */
			break;

    case K_FIELD:	UError ("Sorry, you can't access a naked field");

    } /* switch */

    ty->td.st	 = kind;	/* note symbol type (kind)	  */
    ty->td.width = 0;		/* must be zero except for fields */

} /* TyFHp */


/***********************************************************************
 * T Y	 F   F I E L D	 H P
 *
 * Given a symbol index for a structure and the name of a field (in ty),
 * return the type, offset, and width information for the field in ty.
 *
 * Inits vwidthHp == 0 so it's valid after return from TyFScanHp().
 *
 * This routine sets visym to a new value and then uses it.  It does not
 * reset it, for efficiency reasons.  It also does not use current symbol
 * values after calling TyFScanHp(), which changes visym again.
 */

local void TyFFieldHp (ty, isym)
    register pTYR	ty;		/* field type info to return	*/
    long	isym;			/* of parent struct or union	*/
{
    char	*sbField;		/* name of the field		*/
    DNTTPOINTER nextfield;		/* next field or variant	*/
    FLAGT	fFound = false;		/* did we find the field?	*/
    long	offset;			/* field bit offset		*/

    sbField = SbInCore (ty->sbVar);
    SetSym (isym);			/* get the parent struct or union */

    if ((vsymCur->dblock.kind == K_TAGDEF) ||
      (vsymCur->dblock.kind == K_TYPEDEF))
	SetSym(vsymCur->dtag.type.dnttp.index);

    if (vsymCur->dblock.kind == K_STRUCT)	/* start the field chain */
	nextfield = vsymCur->dstruct.firstfield;
    else
	nextfield = vsymCur->dunion.firstfield;

/*
 * SCAN FIELD CHAIN:
 *
 * The name search allows substring match, the same as TyFField().
 */

    while (nextfield.word != DNTTNIL)
    {
	SetSym (nextfield.dnttp.index);		/* assume next is FIELD */

	if (fFound = FNameCmp (sbField, FSbCmp))	/* found it */
	    break;

	nextfield = vsymCur->dfield.nextfield;		/* try the next one */
    }

    /* doesn't yet handle STRUCT vartagfield nor fields under varlist */

/*
 * FIELD FOUND (otherwise, returns bt == btNil):
 */

    if (fFound)
    {
	offset	 = vsymCur->dfield.bitoffset;
	vwidthHp = 0;					/* base  width */

	TyFScanHp (ty, vsymCur->dfield.type);		/* get type and width */

	ty->valTy    = offset;				/* in bits	     */
	ty->td.width = 0;
    }
} /* TyFFieldHp */



/***********************************************************************
 * C O P Y   T Y
 *
 * Copy type information.  This is done here (instead of just with
 * struct assigns) because ty information is in an array.
 */

export void CopyTy (tyDest, tySrc)
    pTYR	tyDest;		/* result array */
    pTYR	tySrc;		/* source array */
{
    int		cTy;		/* array index	*/

    for (cTy = 0; cTy < cTyMax; cTy++)
	*tyDest++ = *tySrc++;

} /* CopyTy */


/***********************************************************************
 * T Y	 F   G L O B A L
 *
 * Get type information for a global var.
 * Some versions call routines which change visym,
 * so they save it and reset it afterwards.
 */

export void TyFGlobal (rgTy)
    pTYR	rgTy;		/* type info to get */
{
    long	isymSave = visym;

    TyFHp  (rgTy, visym);
    SetSym (isymSave);

} /* TyFGlobal */


/***********************************************************************
 * T Y	 F   L O C A L
 *
 * Get type information for a local var with the given name.
 * Some versions call routines which change visym, so they save it and
 * reset it afterwards.
 */

export void TyFLocal (rgTy, isym)
    pTYR	rgTy;		/* type info to get */
    long	isym;		/* current symbol   */
{
    long	isymSave = isym;

    SetSym (isym);
    TyFHp  (rgTy, isym);
    SetSym (isymSave);

} /* TyFLocal */


/***********************************************************************
 * T Q	 F   T Y
 *
 * Hide details of TYR by returning the desired type qualifier.
 */

export int TqFTy (ty, cnt)
    register pTYR	ty;	/* type to check   */
    int		cnt;	/* which qualifier */
{

    switch (cnt)
    {
	case 1: return (ty->td.tq1);
	case 2: return (ty->td.tq2);
	case 3: return (ty->td.tq3);
	case 4: return (ty->td.tq4);
	case 5: return (ty->td.tq5);
	case 6: return (ty->td.tq6);
    }
    /* NOTREACHED */

} /* TqFTy */


/***********************************************************************
 * A D J   T D
 *
 * Either "push" a new type qualifier (if given), or "pop" the top tq (if
 * none is given), to/from the given type descriptor.  Give an error if
 * there are already too many when trying to add one.
 *
 * For HPSYMTAB, we set/clear/advance the DNTT index to the corresponding
 * symbol table record.  It's only safe to do this when pushing or popping
 * a tq != tqNil (to avoid confusion with DXTYR use of XTYR).
 */

export void AdjTd (ty, tq, isym)
    pTYR	ty;	/* type to adjust	  */
    int		tq;	/* tq to add if not tqNil */
    long	isym;	/* DNTT index of type info for tq -- HPSYMTAB only */
{
    register pTDR	td = & (ty->td);	/* type descriptor ptr */
    register pXTYR	xty = (pXTYR) (ty + 1);	/* extension record */

/*
 * PUSH NEW QUALIFIER:
 */

    if (tq != tqNil)
    {
	if (td->tq6 != tqNil)		/* all seats are taken */
	{
	    Panic ("Type information overflow (more than 6 type qualifiers)");
	}
	else
	{
	    td->tq6 = td->tq5;		/* push down current ones */
	    td->tq5 = td->tq4;
	    td->tq4 = td->tq3;
	    td->tq3 = td->tq2;
	    td->tq2 = td->tq1;
	    td->tq1 = tq;		/* and add new one */

	    xty->st	= stExtend;
	    xty->isymTq	= isym;		/* record DNTT index for new tq	*/
	}
    }
    else
    
/*
 * POP TOP QUALIFIER:
 */

    {
	td->tq1 = td->tq2;
	td->tq2 = td->tq3;
	td->tq3 = td->tq4;
	td->tq4 = td->tq5;
	td->tq5 = td->tq6;
	td->tq6 = tqNil;

	if (td->tq1 == tqNil)	  		/* no more qualifiers */
	{
	    xty->isymTq = isymNil;	  	/* clear symbol ptr */
	}
	else if (xty->isymTq != isymNil)	/* not nil symbol ptr	   */
	{
	    xty->isymTq = IsymFNextTqSym (xty->isymTq);	/* advance symbol ptr */
	}
    }


} /* AdjTd */


/***********************************************************************
 * C B	 F   T Y
 *
 * Return the size in bytes for the given type.  If there is any tqPtr
 * or tqFunc qualifier, use CBPOINT.  If the top tq is a tqArray and
 * fFullArray, use the total array size, else the base element size.  If
 * it's a bit field, use -1.  If it's a nil ty or unknown bt, use CBINT.
 */

export int CbFTy (ty, fFullArray)
    pTYR	ty;		/* type to check	*/
    int		fFullArray;	/* full array, not bt?	*/
{
    register int		itq;		/* index of current tq	*/
    register int		tq;		/* current tq		*/
    register pXTYR	xty = (pXTYR) (ty + 1);	/* exten record	*/

    if (ty == tyNil)
	return (CBINT);

    for (itq = 1; itq <= 6; itq++)
    {
	if ((tq = TqFTy (ty, itq)) == tqNil)	/* no more tq's */
	    break;

	if ((tq == tqPtr) OR (tq == tqFunc))	/* a pointer */
	    return (CBPOINT);

	if ((tq == tqArray)			/* an array	   */
	AND fFullArray				/* want full size  */
	AND (xty->isymTq != isymNil))		/* have isym index */
	{
		SetSym (xty->isymTq);

	        return (vsymCur->darray.arrayisbytes
			? (vsymCur->darray.arraylength)
			: (vsymCur->darray.arraylength / SZCHAR));
	}

    }
    if (ty->td.width != 0)			/* a bit field */
	return (-1);

    switch (ty->td.bt)				/* every bt should be here */
    {
	case btNil:	return (CBINT);		/* take a guess	*/
	/* OK for HPSYMTAB extended types too */

	case btInt:
	case btUInt:	return (CBINT);

	case btUShort:
	case btShort:	return (CBSHORT);

	case btLong:
	case btULong:	return (CBLONG);

	case btChar:
	case btUChar:	return (CBCHAR);

	case btFloat:	return (CBFLOAT);
	case btDouble:	return (CBDOUBLE);

	case btFArg:
	case btEMember: return (CBINT);

	case btEType:
	case btUnion:
	case btStruct:	/* stLeng does not apply to some symbol tables */

			return ((xty->st == stExtend) ? xty->cb     :
				(xty->st == stLeng)   ? ty[1].valTy : CBINT);

    } /* switch */

    return (CBINT);

} /* CbFTy */



/***********************************************************************
 * I S Y M   F   N E X T   T Q   S Y M
 *
 * Find the next DNTT symbol corresponding to a tq modifier.
 * Start with the given symbol index, assumed to correspond to the
 * current modifier. 
 */

export long IsymFNextTqSym (isym)
    long	isym;		/* DNTT index of type info for qual */
{
    DNTTPOINTER nexttype;	/* ptr to next DNTT type rec	*/
    KINDTYPE	kind;		/* kind of next DNTT type rec	*/

    SetSym (isym);	  	/* set to current tq sym */
    kind = vsymCur->dblock.kind;

    do {
	switch (kind)
	{
	    default:		Panic (vsbErrSinSDD, "Wrong symbol type",
					"IsymFNextTqSym", kind, visym);

	    case K_TYPEDEF:	nexttype = vsymCur->dtype.type;		break;
	    case K_TAGDEF:	nexttype = vsymCur->dtag.type;		break;
	    case K_POINTER:	nexttype = vsymCur->dptr.pointsto;	break;
	    case K_SUBRANGE:	nexttype = vsymCur->dsubr.subtype;	break;
	    case K_ARRAY:	nexttype = vsymCur->darray.elemtype;	break;
	    case K_FILE:	nexttype = vsymCur->dfile.elemtype;	break;
	    case K_FUNCTYPE:	nexttype = vsymCur->dfunctype.retval;	break;
	}

	if (nexttype.dnttp.immediate)
	    return (isymNil);

	SetSym (nexttype.dnttp.index);	/* advance DNTT pointer	*/
	kind = vsymCur->dblock.kind;

    } while ((kind != K_POINTER)
      AND    (kind != K_ARRAY)
      AND    (kind != K_FUNCTYPE));

    return (nexttype.dnttp.index);

} /* IsymFNextTqSym */


/***********************************************************************
 * D I M   F   T Y
 *
 * Given the type info for a variable, return array dimension info
 * for the specified qualifier.  Note that tyArray is not necessarily
 * an array in fact.
 *
 * This procedure hides the details of accessing FORTRAN arrays in
 * column-major storage order.  FORTRAN multi-dimension arrays are
 * represented in the DNTT as array entries chained in reverse order,
 * e.g., a(3,4) is "array 1:4 of array 1:3 of <type>".  Thus dimension
 * information must be returned for the DNTT array entry relative to the
 * end of the chain, unlike C and Pascal arrays.
 *
 * The parameter "iTqPos" indicates for which td modifier the calling
 * routine wants array dimension info.  C and Pascal versions of the
 * debugger will always call with positive values of "iTqPos".  ONLY a
 * FORTRAN version will pass negative values of "iTqPos"; these indicate
 * which dimension (in reverse order) to return data for.
 */

export void DimFTy (tyArray, iTqPos, cbArray, lbArray, hbArray)
    pTYR	tyArray;	/* TYR record describing array		*/
    long	iTqPos;		/* relative tq count for which info is  */
				/*   desired:  + values = position from */
				/*   beginning of list (first = 1)	*/
				/*   - values = position from end of	*/
				/*   list (last item = -1)		*/
    long	*cbArray;	/* size in bytes of array element	*/
    long	*lbArray;	/* lower bound of this dimension	*/
    long	*hbArray;	/* high bound of this dimension		*/
{
    pXTYR	xty = (pXTYR) (tyArray + 1);	/* quick pointer	*/
    DNTTPOINTER	dnttElem;			/* elem type DNTT ptr	*/
    DNTTPOINTER	dnttSub;			/* subscript DNTT ptr	*/
    long	isym;				/* index into DNTT	*/
    register long	itq;			/* tq index		*/
    register long	tq;			/* tq qualifier		*/

/*
 * If the caller wants dimension information relative to the tail
 * of the DNTT chain, convert the modifier index to start-relative.
 */

    if (iTqPos < 0)				/* want rel. to end */
    {
	for (itq = 1; itq <= 6; itq++)		/* count modifiers */
	{
	    if ((tq = TqFTy (tyArray, itq)) == tqNil)
		break;

	    if (tq != tqArray)
		UError ("FORTRAN variable not pure array");
	}
	iTqPos += itq;				/* relative to start */
    }

/*
 * Set language-dependent defaults if there is no symbol information.
 * Note that this lets us index off simple variables and constants.
 * It's only safe to use xty if tq1 != tqNil (e.g. it's not a DXTYR).
 */

    if ((TqFTy (tyArray, 1) == tqNil)		/* no qualifiers	*/
    OR  (xty->st != stExtend)			/* no extension info	*/
    OR  ((isym = xty->isymTq) == isymNil))	/* no symbol info	*/
    {
	*lbArray = 0;				/* C starts 0, others at 1 */
	*hbArray = intMax;
	*cbArray = CbFTy (tyArray, false);	/* no choice: use base size */
	return;
    }

/*
 * Find DNTT entry corresponding to the indicated tq modifier
 */

    SetSym (isym);

    for (itq = iTqPos - 1; itq > 0; itq--)	/* find specified DNTT	*/
    {						/* record from iTqPos	*/
	if ((isym = IsymFNextTqSym (isym)) == isymNil)
	    UError ("Too many subscripts");
    }

/*
 * Return dimension information from the DNTT record.
 * We must be looking at an DNTT ARRAY-type record except if this
 * is a C-debugger and the corresponding modifier is a pointer.  (We
 * allow valid C syntax of indexing off a pointer.)
 */

    if (vsymCur->dblock.kind == K_ARRAY)
    {
	dnttSub  = vsymCur->darray.indextype;
	dnttElem = vsymCur->darray.elemtype;

	*cbArray = vsymCur->darray.elemlength;
	if (vsymCur->darray.elemisbytes == 0)
	    *cbArray /= SZCHAR;

	BoundsFDntt (dnttSub, lbArray, hbArray);	/* determine array bounds */

	if (*cbArray == 0)
	    *cbArray = CbFDntt (dnttElem);		/* bytes from DNTT entry  */
    }
    else
	if ((vsymCur->dblock.kind == K_POINTER)
	AND (TqFTy (tyArray, 1) == tqPtr))	/* supports [] of ptr in C */
	{
	    *lbArray = 0;
	    *hbArray = intMax;

	    dnttElem = vsymCur->dptr.pointsto;  /* get size of object pointed */
	    *cbArray = CbFDntt (dnttElem);	/*    at from DNTT pointer    */
	}
        else			/* not ARRAY or valid POINTER DNTT entry */
	    UError ("Too many subscripts");

}  /* DimFTy */


/****************************************************************************
 * C B   F   D N T T
 *
 * Given a DNTTPOINTER, return the size (in bytes) of the type it describes.
 */

local long CbFDntt (dnttIn)
    DNTTPOINTER	dnttIn;		/* input DNTT type ptr; maybe immediate */
{
    long	isym;		/* DNTT index 			*/
    KINDTYPE	kind;		/* kind of DNTT entry		*/
    DNTTPOINTER dnttPtr;	/* ptr to current DNTT entry	*/
    int		cb = -1;	/* result size			*/

    dnttPtr.word = dnttIn.word;

    do {
	if (dnttPtr.dnttp.immediate)
	    return (dnttPtr.dntti.bitlength / SZCHAR);

	isym = dnttPtr.dnttp.index;
	SetSym (isym);			/* now looking at chained type */
	kind = vsymCur->dblock.kind;

	switch (kind)
	{
	    default:		Panic (vsbErrSinSDD, "Wrong symbol type",
					"CbFDntt", kind, isym);

	    case K_TYPEDEF:	dnttPtr = vsymCur->dtype.type;		break;
	    case K_TAGDEF:	dnttPtr = vsymCur->dtag.type;		break;

	    case K_POINTER:	cb = vsymCur->dptr.bitlength;		break;
	    case K_ENUM:	cb = vsymCur->denum.bitlength;		break;
	    case K_SET:		cb = vsymCur->dset.bitlength;		break;
	    case K_SUBRANGE:	cb = vsymCur->dsubr.bitlength;		break;
	    case K_ARRAY:	if (vsymCur->darray.arrayisbytes) {
				  cb = SZCHAR * vsymCur->darray.arraylength;
				}
				else {
				  cb = vsymCur->darray.arraylength;
				}
				if (cb == 0) {
				   int lb,hb,elemsize;
				   DNTTPOINTER dnttElem;
				   DNTTPOINTER dnttSub;
				   dnttSub  = vsymCur->darray.indextype;
				   dnttElem = vsymCur->darray.elemtype;
				   elemsize = CbFDntt(dnttElem, true);
				   BoundsFDntt (dnttSub, &lb, &hb);
				   cb = (hb - lb + 1) * elemsize * SZCHAR;
				}
				break;
	    case K_STRUCT:	cb = vsymCur->dstruct.bitlength;	break;
	    case K_UNION:	cb = vsymCur->dunion.bitlength;		break;
	    case K_FILE:	cb = vsymCur->dfile.bitlength;		break;
	    case K_FUNCTYPE:	cb = vsymCur->dfunctype.bitlength;	break;
	}

    } while (cb < 0);

    return (cb / SZCHAR);

}  /* CbFDntt */



/***********************************************************************
 * B O U N D S   F   D N T T
 *
 * Determine bounds of an array from the subscript DNTT
 */

static void BoundsFDntt (dnttSub, lbArray, hbArray)
    DNTTPOINTER dnttSub;		/* Subscript DNTT */
    long	*lbArray;		/* lower bound (returned) */
    long	*hbArray;		/* upper bound (returned) */
{

    if (dnttSub.dnttp.immediate)
    {
	Panic (vsbErrSinSDD, "Immediate subscript type", "BoundsFDntt",
		dnttSub.dnttp.immediate, visym);
    }
    else				/* subscript not immediate type */
    {
	SetSym (dnttSub.dnttp.index);	/* now looking at subscript */

	if ((vsymCur->dblock.kind == K_TAGDEF) ||	/* really looking at typedef? */
	  (vsymCur->dblock.kind == K_TYPEDEF))
	    SetSym(vsymCur->dtag.type.dnttp.index);	/* dereference typedef */

	if (vsymCur->dblock.kind = K_SUBRANGE)
	{
	    *lbArray = vsymCur->dsubr.lowbound;
	    *hbArray = vsymCur->dsubr.highbound;
	}
	else
	    Panic (vsbErrSinSDD, "Subscript not subrange type", "BoundsFDntt",
		vsymCur->dblock.kind, visym);
    }

} /* BoundsFDntt */
