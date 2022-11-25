/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/ty.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:23:59 $
 */

/*
 * Original version based on: 
 * Revision 63.8  88/05/27  08:19:09  08:19:09  sjl (Steve Lilker)
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * This large collection of routines is essentially used to build internal
 * type records (TYRs) from external symbol records (SYMRs) from the object
 * file.  Most of the knowledge about different symbol table structures is
 * concentrated here.
 */

#include <ctype.h>
#include "cdb.h"

#define crockValue (-1)			/* nil name for type		*/
#define cbSbSafe   BUFSIZ		/* size of misc. string cache	*/

static char vsbSafe[cbSbSafe];		/* misc. string cache	*/

#ifdef XDB
FLAGT vfgettingfieldtype = false;
FLAGT vffieldtypeispointer = false;
#endif

#ifdef HPSYMTABII
CTR vrgDims[100]; /* table of cobol table bounds and byte size */
int viDim;        /* index of cobol table for current field    */
int viDimMac;     /* number of cobol tables for field list     */
#endif


/***********************************************************************
 * STANDARD TYPE RECORDS:
 *
 * These are a list of instances of each type, followed by pointers to
 * each type.  The boolean field is fConstant.
 *
 * Note that dummy extension records are NOT allocated after each TYR.
 * If someone copies a vty using CopyTy(), the second record is "junk".
 */

static TYR	atyZeros;			/* defaults to all zeros */

#ifdef M68000
static TYR	atyCnChar  = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btChar,  0, true, 0};
static TYR	atyChar	   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btChar,  0, false, 0};
/*
static TYR	atyUChar   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btUChar, 0, false, 0};
static TYR	atyCnShort = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btShort, 0, true, 0};
static TYR	atyShort   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btShort, 0, false, 0};
static TYR	atyUShort  = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btUShort,0, false, 0};
*/
static TYR	atyCnInt   = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btInt,   0, true, 0};
static TYR	atyInt	   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btInt,   0, false, 0};
static TYR	atyCnLong  = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btLong,  0, true, 0};
static TYR	atyLong	   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btLong,  0, false, 0};
/*
static TYR	atyULong   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btULong, 0, false, 0};
static TYR	atyCnFloat = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btFloat, 0, true, 0};
static TYR	atyFloat   = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btFloat, 0, false, 0};
*/
static TYR	atyCnDouble= {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btDouble,0, true, 0};
static TYR	atyDouble  = {crockValue,  stValue, false, 0, 0,0,0,0,0,0, 
			btDouble,0, false, 0};
#ifdef XDB
static TYR	atyCnBool  = {crockValue,  stValue, true,  0, 0,0,0,0,0,0, 
			btULong, 0, true, 0};
static XTYR	atyCnBoolX = {0, stExtend, TX_BOOL, 0, 0};
#endif
#else
static TYR	atyCnChar  = {crockValue,  stValue, true,  0, btChar,  
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyChar	   = {crockValue,  stValue, false, 0, btChar,  
			0,0,0,0,0,0, 0, false, 0};
/*
static TYR	atyUChar   = {crockValue,  stValue, false, 0, btUChar, 
			0,0,0,0,0,0, 0, false, 0};
static TYR	atyCnShort = {crockValue,  stValue, true,  0, btShort, 
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyShort   = {crockValue,  stValue, false, 0, btShort, 
			0,0,0,0,0,0, 0, false, 0};
static TYR	atyUShort  = {crockValue,  stValue, false, 0, btUShort,
			0,0,0,0,0,0, 0, false, 0};
*/
static TYR	atyCnInt   = {crockValue,  stValue, true,  0, btInt,   	
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyInt	   = {crockValue,  stValue, false, 0, btInt,   
			0,0,0,0,0,0, 0, false, 0};
static TYR	atyCnLong  = {crockValue,  stValue, true,  0, btLong,  
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyLong	   = {crockValue,  stValue, false, 0, btLong,  
			0,0,0,0,0,0, 0, false, 0};
/*
static TYR	atyULong   = {crockValue,  stValue, false, 0, btULong, 
			0,0,0,0,0,0, 0, false, 0};
static TYR	atyCnFloat = {crockValue,  stValue, true,  0, btFloat, 
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyFloat   = {crockValue,  stValue, false, 0, btFloat, 
			0,0,0,0,0,0, 0, false, 0};
*/
static TYR	atyCnDouble= {crockValue,  stValue, true,  0, btDouble,
			0,0,0,0,0,0, 0, true, 0};
static TYR	atyDouble  = {crockValue,  stValue, false, 0, btDouble,
			0,0,0,0,0,0, 0, false, 0};
#ifdef XDB
static TYR	atyCnBool  = {crockValue,  stValue, true,  0, btULong, 
			0,0,0,0,0,0, 0, true, 0};
static XTYR	atyCnBoolX = {0, stExtend, TX_BOOL, 0, 0};
static TYR	atyCnCobol  = {crockValue,  stValue, true,  0, btNil, 
			0,0,0,0,0,0, 0, true, 0};
static XTYR	atyCnCobolX = {-1, stExtend, TX_COBOL, 0, 0};
#endif
#endif /* not M68000 */

pTYR	vtyZeros   = & atyZeros;
pTYR	vtyCnChar  = & atyCnChar;
pTYR	vtyChar	   = & atyChar;
/*
pTYR	vtyUChar   = & atyUChar;
pTYR	vtyCnShort = & atyCnShort;
pTYR	vtyShort   = & atyShort;
pTYR	vtyUShort  = & atyUShort;
*/
pTYR	vtyCnInt   = & atyCnInt;
pTYR	vtyInt	   = & atyInt;
pTYR	vtyCnLong  = & atyCnLong;
pTYR	vtyLong	   = & atyLong;
/*
pTYR	vtyULong   = & atyULong;
pTYR	vtyCnFloat = & atyCnFloat;
pTYR	vtyFloat   = & atyFloat;
*/
pTYR	vtyCnDouble= & atyCnDouble;
pTYR	vtyDouble  = & atyDouble;
#ifdef XDB
pTYR	vtyCnBool  = & atyCnBool;
#ifdef HPSYMTABII
pTYR	vtyCnCobol  = & atyCnCobol;
#endif
#endif


/***********************************************************************
 * TYPE INFORMATION VARS AND CODE:
 *
 * Most of this stuff is NOT needed if we have BSD or HP symbol tables.
 */

/***********************************************************************
 * S B	 S A F E
 *
 * Stick a string in a temporarily safe place and return a pointer to it.
 */

SBT SbSafe (sb)
    char	  *sb;		/* string to save */
{
    register char *sbT = sb;	/* temp ptr	 */
    register long  cb;		/* bytes to save */
    SBT		   sbRet;	/* ptr to return */
    static char	  *vsbNextSafe = vsbSafe;	/* next available place */

#ifdef INSTR
    vfStatProc[378]++;
#endif

    while (*sbT != chNull)			/* find null */
	sbT++;

    cb = sbT - sb + 1;				/* bytes to save */

    if (cb > cbSbSafe)
    {
	sprintf (vsbMsgBuf, (nl_msg(560, "Internal Error (IE560) (%d)")), cb);
	Panic (vsbMsgBuf);
    }

    if (vsbNextSafe + cb >= vsbSafe + cbSbSafe) /* passes end of buffer */
	vsbNextSafe = vsbSafe;			/* reuse the buffer	*/

    strncpy (vsbNextSafe, sb, cb);

    sbRet = bitHigh | (SBT) vsbNextSafe;	/* mark as char*, not iss */

    vsbNextSafe [cb - 1] = chNull;		/* add terminator */
    vsbNextSafe += cb;

    return (sbRet);

} /* SbSafe */


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
#ifdef HPSYMTABII
    TYR		ty[cTyMax];		/* for nonconstant bounds */
    ADRT	adr;
    long	isym;
    STUFFU      stuff;
#endif

#ifdef INSTR
    vfStatProc[379]++;
#endif

    if (dnttSub.dnttp.immediate)
    {
	if (dnttSub.dntti.type == T_CHAR)
	{
	    *lbArray = 0;
	    *hbArray = 255;
	}
        else if (dnttSub.dntti.type == T_BOOLEAN)
	{
	    *lbArray = 0;
	    *hbArray = 1;
	}
	else
	{
	    sprintf (vsbMsgBuf,
		     (nl_msg(561, "Internal Error (IE561) (%d, %d)")),
		     dnttSub.dnttp.immediate, visym);
	    Panic (vsbMsgBuf);
	}
    }
    else				/* subscript not immediate type */
    {
	SetSym (dnttSub.dnttp.index);	/* now looking at subscript? */

        if (vsymCur->dblock.kind == K_TYPEDEF)   /* really looking at typedef */
            SetSym (vsymCur->dtype.type.dnttp.index);  /* dereference typedef */

	if (vsymCur->dblock.kind == K_SUBRANGE)
	{
#ifdef HPSYMTABII
            switch (vsymCur->dsubr.dyn_low) {

               case 0:
	          *lbArray = vsymCur->dsubr.lowbound;
                  break;
               case 1:
                  *lbArray = GetWord(vsp + vsymCur->dsubr.lowbound);
                  break;
               case 2:
                  isym = vsymCur->dsubr.lowbound & 0x1fffffff;
                  SetSym(isym);
                  TyFHp(ty,isym);
                  adr = AdrFIsym(isym, vsp, vsp, vpc, tyNil);
                  ValFAdrTy(&stuff, adr, ty, 0, 0);
                  if (ty->td.bt ==  btFloat) {
                     *lbArray = stuff.fl;
                  }
                  else if (ty->td.bt ==  btDouble) {
                     *lbArray = stuff.doub;
                  }
                  else {
                     *lbArray = stuff.lng;
                  }
                  break;

            }
            switch (vsymCur->dsubr.dyn_high) {

               case 0:
	          *hbArray = vsymCur->dsubr.highbound;
                  break;
               case 1:
                  *hbArray = GetWord(vsp + vsymCur->dsubr.highbound);
                  break;
               case 2:
                  isym = vsymCur->dsubr.highbound & 0x1fffffff;
                  SetSym(isym);
                  TyFHp(ty,isym);
                  adr = AdrFIsym(isym, vsp, vsp, vpc, tyNil);
                  ValFAdrTy(&stuff, adr, ty, 0, 0);
                  if (ty->td.bt ==  btFloat) {
                     *hbArray = stuff.fl;
                  }
                  else if (ty->td.bt ==  btDouble) {
                     *hbArray = stuff.doub;
                  }
                  else {
                     *hbArray = stuff.lng;
                  }
                  break;

            }
#else
	    *lbArray = vsymCur->dsubr.lowbound;
	    *hbArray = vsymCur->dsubr.highbound;
#endif
	}
	else if (vsymCur->dblock.kind == K_ENUM)
	{

	/*
	 * Find bounds of enumeration values
	 */
	    SetSym (vsymCur->denum.firstmem.dnttp.index);
	    *lbArray = vsymCur->dmember.value;
	    while (vsymCur->dmember.nextmem.word != isymNil)
		SetSym (vsymCur->dmember.nextmem.dnttp.index);
	    *hbArray = vsymCur->dmember.value;
	}
	else
	{
	    sprintf (vsbMsgBuf,
		     (nl_msg(562, "Internal Error (IE562) (%d, %d)")),
		     vsymCur->dblock.kind, visym);
	    Panic (vsbMsgBuf);
	}
    }
} /* BoundsFDntt */


/***********************************************************************
 * S E T   W I D T H   (macro)
 *
 * If the type has not already picked up a width (the first one seen
 * while traipsing down the type chain), set the given width.  Since
 * the ty only provides room for 7 bits, we use a separate, global var
 * to hold the width.
 */

static  long vwidthHp;		/* highest level width */
long	vbtWidthHp;		/* base type width */

#define SetWidth(wide)   {if (vwidthHp == 0) vwidthHp = (vbtWidthHp = wide);}


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

void TyFImmedHp (ty, di)
    pTYR	ty;			/* field type info to return	 */
    struct DNTTP_IMMEDIATE di;		/* this immediate value		 */
{
    register ulong width = di.bitlength;	/* bit size of object	*/
    register int   bt;				/* equiv. base type	*/
    pXTYR	   xty = (pXTYR) (ty + 1); 	/* quick pointer	*/

#ifdef INSTR
    vfStatProc[380]++;
#endif

    switch (di.type)
    {

    default:		sprintf (vsbMsgBuf,
			         (nl_msg(563, "Internal Error (IE563) (%d, %d)")), di.type, visym);
			Panic (vsbMsgBuf);

    case T_UNDEFINED:	/* take a guess */
			if	(width <= SZCHAR)	bt = btChar;
			else if	(width <= SZSHORT)	bt = btShort;
			else				bt = btInt;
			break;

#ifdef HPSYMTABII
    case T_BOOLEAN_S300_COMPAT:
    case T_BOOLEAN_VAX_COMPAT:
#endif
    case T_BOOLEAN:	if	(width == 1) {
							bt = btULong;
#ifdef SPECTRUM
                        /* fixes pascal bug */          di.bitlength = SZLONG;
#endif
					     }
			else if	(width == SZCHAR)	bt = btUChar;
			else if	(width == SZSHORT)	bt = btUShort;
			else if	(width == SZLONG)	bt = btULong;
			else
			{
			     sprintf (vsbMsgBuf,
			     (nl_msg(564, "Internal Error (IE564) (%d, %d)")),
				      width, visym);
			     Panic (vsbMsgBuf);
			}
#ifdef SPECTRUM
                        xty->st = stExtend;
                        switch (di.type) {
                           case T_BOOLEAN_S300_COMPAT:
                              xty->btX = TX_BOOL_S300;
                              break;
                           case T_BOOLEAN_VAX_COMPAT:
                              xty->btX = TX_BOOL_VAX;
                              break;
                           case T_BOOLEAN:
                              xty->btX = TX_BOOL;
                              break;
                        }
#endif
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
			else if (width == 96)		/* magic number */
				Panic ((nl_msg(565, "Can't handle decimal reals (IE565)")));
			else
			{
				sprintf (vsbMsgBuf,
			(nl_msg(566, "Internal Error (IE566) (%d, %d)")), width, visym);
				Panic (vsbMsgBuf);
			}
			break;

#ifdef HPSYMTABII
    case T_FTN_STRING_SPEC:
    case T_MOD_STRING_SPEC:
    case T_FTN_STRING_S300_COMPAT:
#ifdef XDB
    case T_PACKED_DECIMAL:
#endif
#endif	/* HPSYMTABII */
#ifdef S200
    case T_STRING200:
    case T_LONGSTRING200:
#endif
#ifdef FOCUS
    case T_STRING500:
#endif
    case T_TEXT:
    case T_FLABEL:	bt	     = btNil;	/* special case */
			xty->st	     = stExtend;
			xty->btX     = di.type; /* hide HPSYMTABs type here */
			xty->isymRef = visym;	/* remember the symbol too */
			break;

#ifdef HPSYMTABII
    case T_GLOBAL_ANYPTR:
    case T_LOCAL_ANYPTR:
    case T_ANYPTR:	bt = btInt;
                        AdjTd(ty,tqPtr,visym);
			break;
#endif

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

void TyFScanHp (ty, dp)
    register pTYR  ty;			/* field type info to return	*/
    DNTTPOINTER    dp;			/* next symbol to handle	*/
{
    register pXTYR xty = (pXTYR) (ty + 1); /* quick pointer		*/
    KINDTYPE	   kind;		/* of this symbol		*/
    long	   isym;		/* if it's an index, not immed	*/

#ifdef INSTR
    vfStatProc[381]++;
#endif

    if (dp.word == DNTTNIL) {			/* absent type info case */
#ifdef XDB
        if (vfgettingfieldtype) {
           vfgettingfieldtype = false;
        }
#endif
	return;
    }

    if (dp.dntti.immediate)			/* immediate case */
    {
#ifdef XDB
        if (vfgettingfieldtype) {
           vfgettingfieldtype = false;
        }
#endif
	TyFImmedHp (ty, dp.dntti);
	return;
    }

/*
 * NON-IMMEDIATE CASE:
 */

    isym = dp.dnttp.index;
    SetSym (isym);
    kind = vsymCur->dblock.kind;
#ifdef XDB
        if (vfgettingfieldtype) {
           vfgettingfieldtype = false;
           if (kind == K_POINTER) {
              vffieldtypeispointer = true;
           }
        }
#endif

    switch (kind)
    {

    default:		sprintf (vsbMsgBuf,
				 (nl_msg(567, "Internal Error (IE567)(%d, %d)")), kind, visym);
#ifdef DEBREC
			return;
#endif
    			Panic (vsbMsgBuf);

    case K_TYPEDEF:	TyFScanHp (ty, vsymCur->dtype.type);	break;
    case K_TAGDEF:	TyFScanHp (ty, vsymCur->dtag.type);	break;

    case K_POINTER:	SetWidth  (    vsymCur->dptr.bitlength);
			TyFScanHp (ty, vsymCur->dptr.pointsto);
    			AdjTd	  (ty, tqPtr, isym);
			break;

    case K_ENUM:	ty->td.bt    = btEType;
			xty->st	     = stExtend;
			xty->isymRef = isym;
			if (ty->td.width == 0)	/* packed subrange not found */
			{
#ifdef HPSYMTABII
			    if (vsymCur->denum.bitlength <= SZCHAR) {
			        xty->cb = 1;
                            }
			    else {
			       if (vsymCur->denum.bitlength <= SZSHORT) {
			           xty->cb = 2;
                               }
			       else {
			           xty->cb = 4;
                               }
                            }
#else
			    if (vsymCur->denum.bitlength % SZCHAR)
			        ty->td.width = vsymCur->denum.bitlength;
			    else
			        xty->cb = vsymCur->denum.bitlength / SZCHAR;
#endif
			}
			SetWidth (vsymCur->denum.bitlength);
			break;

    case K_MEMENUM:	ty->td.bt	 = btEMember;
			ty->valTy	 = vsymCur->dmember.value;
			ty->td.fConstant = true;
			ty->fAdrIsVal	 = true;
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

    case K_SUBRANGE:	{
#ifdef HPSYMTABII
			    long  bitlength = vsymCur->dsubr.bitlength;

                            if (bitlength == 3) {
                               bitlength = 4;
                            }
                            else {
                               if ((bitlength > 4) && (bitlength <= 8)) {
                                  bitlength = 8;
                               }
                               else {
                                  if ((bitlength > 8) && (bitlength <= 16)) {
                                     bitlength = 16;
                                  }
                                  else {
                                     if ((bitlength > 16) && (bitlength < 33)) {
                                        bitlength = 32;
                                     }
                                  }
                               }
                            }

			    SetWidth(bitlength);
			    TyFScanHp(ty, vsymCur->dsubr.subtype);
#else /* HPSYMTAB */
			    long bitlength = vsymCur->dsubr.bitlength;

			    TyFScanHp(ty, vsymCur->dsubr.subtype);
			    if ((bitlength % SZCHAR)
			     OR (bitlength < vbtWidthHp))  /* if odd size or */
					     /* base type thinks it's longer */
			    {
			        ty->td.width = bitlength;
			    }
#endif
			    break;
			}

    case K_ARRAY:	{
#ifdef HPSYMTAB
                           SetWidth(vsymCur->darray.bitlength);
#else  /* HPSYMTABII */
                           long bitlength = vsymCur->darray.elemlength;
                           if (vsymCur->darray.elemisbytes) {
                              bitlength *= SZCHAR;
                           }
   
                           if (vsymCur->darray.arrayisbytes) {
                              SetWidth(SZCHAR * vsymCur->darray.arraylength);
                           }
                           else {
                              SetWidth(vsymCur->darray.arraylength);
                           }
#endif /* HPSYMTABII */
			   TyFScanHp (ty, vsymCur->darray.elemtype);
    			   AdjTd (ty, tqArray, isym);
#ifdef HPSYMTABII
                           if ((bitlength < vbtWidthHp) || 
                               (bitlength % SZCHAR)) {
                              ty->td.width = bitlength;
                           }
#endif
			   break;
                        }

    case K_STRUCT:	ty->td.bt    = btStruct;
			xty->st	     = stExtend;
			xty->isymRef = isym;
			xty->cb	     = vsymCur->dstruct.bitlength / SZCHAR;
			SetWidth (vsymCur->dstruct.bitlength);
			break;

    case K_UNION:	ty->td.bt    = btUnion;
			xty->st	     = stExtend;
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

#ifdef HPSYMTABII
    case K_COBSTRUCT:	ty->td.bt    = btNil;	/* special case */
			xty->st	     = stExtend;
			xty->btX     = TX_COBOL;/* hide pseudo base type here */
			xty->isymRef = visym;	/* remember the symbol too    */
			xty->cb      = vsymCur->dcobstruct.bitlength;
			break;
#endif
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
 * Note that fields are treated as a special case (not handled here).
 *
 * This routine sets visym to a new value and then uses it.  It does not
 * reset it, for efficiency reasons.  It also does not use current symbol
 * values after calling TyFScanHp(), which changes visym again.
 */

void TyFHp (ty, isym)
    register pTYR ty;			/* field type info to return	*/
    long	  isym;			/* of symbol to handle		*/
{
    pXTYR	xty = (pXTYR) (ty + 1); /* quick pointer	*/
    KINDTYPE	kind;			/* kind of this isym	*/

#ifdef INSTR
    vfStatProc[382]++;
#endif

    ty[0] = ty[1] = *vtyZeros;			/* clear out the type info */
    ty->sbVar = SbSafe ((SBT) NameFCurSym());	/* remember the name	   */
    ty->td.width = 0;				/* initialize: not bit field */
    vwidthHp = 0;				/* initialize global width */

    SetSym (isym);
    kind = vsymCur->dtype.kind;

    switch (kind)
    {

    default:		sprintf (vsbMsgBuf,
				 (nl_msg(568, "Internal Error (IE568) (%d, %d)")), kind, visym);
    			Panic (vsbMsgBuf);

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

    case K_CONST:	ty->td.fConstant = true;
			if ((vsymCur->dconst.locdesc == LOC_IMMED)
			OR  (vsymCur->dconst.locdesc == LOC_VT))
			    ty->fAdrIsVal = true;
			TyFScanHp (ty, vsymCur->dconst.type);	break;

    case K_TYPEDEF:	TyFScanHp (ty, vsymCur->dtype.type);	break;
    case K_TAGDEF:	TyFScanHp (ty, vsymCur->dtag.type);	break;

    case K_MEMENUM:	ty->td.bt	 = btEMember;
			ty->valTy	 = vsymCur->dmember.value;
			ty->td.fConstant = true;
			ty->fAdrIsVal    = true;
			xty->st		 = stExtend;
			xty->isymRef	 = isym;	/* remember where */
			break;

    case K_FIELD:	UError ((nl_msg(452, "Sorry, you can't access a naked field")));

    } /* switch */

    ty->td.st = kind;		/* note symbol type (kind) */

} /* TyFHp */

#ifdef S200
/***********************************************************************
 * F  N A M E  F I N D
 *
 * Given a DNTT pointer to a field type, do a depth first search through
 * structures and unions to find the given name. The search only continues
 * deeper when the current field is an unnamed structure or union.
 */
static FLAGT FNameFind(nextfield, name, total_offset)
DNTTPOINTER nextfield;
char *name;
long *total_offset;
{
    FLAGT fFound = false;
    DNTTPOINTER cdntt;
    long offset;
    char *curName;

#ifdef INSTR
    vfStatProc[420]++;
#endif

    while (nextfield.word != DNTTNIL)
    {
	SetSym (nextfield.dnttp.index);
	if (vsymCur->dfield.bitlength) {
	    offset = vsymCur->dfield.bitoffset;
	    if ((*name != '@') && (curName = SbInCore(vsymCur->dfield.name))
	      && (*curName == '@')) {
		cdntt = vsymCur->dfield.type;
		if (!(cdntt.dntti.immediate || (cdntt.word == DNTTNIL))) {
	            SetSym (cdntt.dnttp.index);
	            if (vsymCur->dstruct.kind == K_STRUCT) {
		        if (fFound = FNameFind(vsymCur->dstruct.firstfield, 
			    name, total_offset)) break;
                    } else if (vsymCur->dunion.kind == K_UNION) {
		        if (fFound = FNameFind(vsymCur->dunion.firstfield, 
			    name, total_offset)) break;
                    }
	            SetSym (nextfield.dnttp.index);
		}
            } else if (fFound = FNameCmp (name, FSbCmp)) 
		break;
	}
	nextfield = vsymCur->dfield.nextfield;		/* try the next one */
    }

    if (fFound) *total_offset += offset;
    return fFound;
} /* FNameFind */
#endif S200

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

void TyFFieldHp (ty, isym)
    register pTYR ty;			/* field type info to return	*/
    long	  isym;			/* of parent struct or union	*/
{
    char	*sbField;		/* name of the field		*/
    DNTTPOINTER nextfield;		/* next field or variant	*/
    DNTTPOINTER vartagfield;		/* tag field for variant	*/
    DNTTPOINTER varlist;		/* next variant			*/
    FLAGT	fFound = false;		/* did we find the field?	*/
    long	offset;			/* field bit offset		*/
    long	width;			/* field bit length		*/
    long	varoffset;		/* accumulated variant offset	*/
    pXTYR	xty = (pXTYR) (ty + 1); /* quick pointer 		*/
    FLAGT	fPacked = false;	/* packed record?		*/

#ifdef INSTR
    vfStatProc[383]++;
#endif

    sbField = SbInCore (ty->sbVar);
    SetSym (isym);			/* get the parent struct or union */

    if (vsymCur->dblock.kind == K_STRUCT)	/* start the field chain */
    {
	nextfield   =	vsymCur->dstruct.firstfield;
	vartagfield =	vsymCur->dstruct.vartagfield;
	varlist	    =	vsymCur->dstruct.varlist;
#ifndef HPSYMTABII
	fPacked	    =	vsymCur->dstruct.ispacked;
#endif
    }
    else
    {
	nextfield	 =	vsymCur->dunion.firstfield;
	vartagfield.word =	DNTTNIL;
	varlist.word	 =	DNTTNIL;
    }

    varoffset = 0;			/* no accumulated offset yet	*/

/*
 * SCAN FIELD CHAIN:
 *
 * The name search allows substring match, the same as TyFField().
 */

#ifdef SPECTRUM
    while (nextfield.word != DNTTNIL)
    {
	SetSym (nextfield.dnttp.index);		/* assume next is FIELD */

	if (fFound = FNameCmp (sbField, FSbCmp))	/* found it */
	    break;

	nextfield = vsymCur->dfield.nextfield;		/* try the next one */
    }
#else
    offset = 0;
    fFound = FNameFind(nextfield, sbField, &offset);
#endif

/*
 * CHECK VARIANT TAG FIELD (IF ANY)
 */

    if ((! fFound) AND 
        (vartagfield.word != DNTTNIL) AND
        (! vartagfield.dntti.immediate))
    {
	SetSym (vartagfield.dnttp.index);
	fFound = FNameCmp (sbField, FSbCmp);
#ifdef S200
	if (fFound) offset = vsymCur->dfield.bitoffset;
#endif
    }

/*
 * FIELD FOUND (otherwise, returns bt == btNil):
 */

    if (fFound)
    {
#ifdef SPECTRUM
	offset	 = vsymCur->dfield.bitoffset;
#endif
	width	 = vsymCur->dfield.bitlength;		/* field width */
	vwidthHp = 0;					/* base  width */
	ty->td.width = 0;

#ifdef XDB
       vfgettingfieldtype = true;
       vffieldtypeispointer = false;
#endif 

	TyFScanHp (ty, vsymCur->dfield.type);		/* get type and width */

	if (ty->td.width != 0)				/* packed array	     */
	{
	    ty->valTy = offset;
	}
	else
	{
#ifndef HPSYMTABII
	    if (width == vwidthHp)			/* not a C bit field */
	    {
#endif
#ifdef HPSYMTABII
#ifdef XDB
	        if ((offset % SZCHAR) OR (width % SZCHAR) OR
                    ((width < vbtWidthHp) && !vffieldtypeispointer))
#else /* CDB */
	        if ((offset % SZCHAR) OR (width % SZCHAR) OR
                    (width < vbtWidthHp))
#endif
#else
	        if ((offset % SZCHAR) OR (width % SZCHAR))  /* bit field if */
#endif
	        {					/* not on byte bndry */
							/* or not byte mult  */
	            ty->valTy    = offset;		/* in bits	     */
	            ty->td.width = width;		/* in bits ( < 128!) */
	        }
	        else
	        {
#ifdef HPSYMTABII
	            ty->valTy    = offset;		/* in bits  */
#else
	            ty->valTy = offset / SZCHAR;	/* in bytes */
#endif
	            ty->td.width = 0;
	        }
#ifndef HPSYMTABII
	    }
	    else
	    {
		if (!fPacked)	   /* not Pascal packed array => C bit field */
		{
	    	    ty->valTy = offset;			/* in bits	     */
	    	    ty->td.width = width;		/* in bits ( < 128!) */
	    	    ty->td.bt = btUInt;			/* always treat thus */
		}
	    }
#endif
	}
    }

/*
 * CHECK ALL VARIANTS
 */

    while ((! fFound) AND (varlist.word != DNTTNIL))
    {
	SetSym (varlist.dnttp.index);
	varoffset = vsymCur->dvariant.bitoffset;
	varlist = vsymCur->dvariant.nextvar;
        if (vsymCur->dvariant.varstruct.word != DNTTNIL) {
	   TyFFieldHp (ty, vsymCur->dvariant.varstruct.dnttp.index);
	   if ((ty->td.bt != btNil) OR (xty->st == stExtend)) {
	       fFound = true;
#ifdef HPSYMTABII
	       ty->valTy += varoffset;
#else
	       if (ty->td.width) {		/* bit offset */
		   ty->valTy += varoffset;
	       }
	       else {			/* byte offset */
		   ty->valTy += varoffset / SZCHAR;
	       }
#endif
	
	   }
	}
    }

} /* TyFFieldHp */



/***********************************************************************
 * C O P Y   T Y
 *
 * Copy type information.  This is done here (instead of just with
 * struct assigns) because ty information is in an array.
 */

void CopyTy (tyDest, tySrc)
    register pTYR	tyDest;		/* result array */
    register pTYR	tySrc;		/* source array */
{
    register int	cTy;		/* array index	*/

#ifdef INSTR
    vfStatProc[384]++;
#endif

    for (cTy = 0; cTy < cTyMax; cTy++)
	*tyDest++ = *tySrc++;

} /* CopyTy */


#ifdef HPSYMTAB
/***********************************************************************
 * T Y	 F   C U R   S Y M
 *
 * Get type information for the current variable (vsymCur)
 * Since isym is given, sbVar is of limited use...
 *
 * Some versions call routines which change visym, so they save it and
 * reset it afterwards.
 */

void TyFCurSym (rgTy)
    pTYR	rgTy;		/* type info to get */
{
    long	isymSave = visym;

#ifdef INSTR
    vfStatProc[385]++;
#endif

    TyFHp  (rgTy, visym);
    SetSym (isymSave);

} /* TyFCurSym */
#endif  /* HPSYMTAB */


#ifdef HPSYMTABII
/***********************************************************************
 * T Y	 F   G L O B A L
 *
 * Get type information for a global var with the given name (may have
 * to search for it).  Some versions call routines which change visym,
 * so they save it and reset it afterwards.
 */

void TyFGlobal (rgTy, sbVar)
    pTYR	rgTy;		/* type info to get */
    char	*sbVar;		/* name to match    */
{
/*
 * We are only called if visym already points to the correct symbol, so
 * there's no need to search.
 */
    long	isymSave = visym;

#ifdef INSTR
    vfStatProc[386]++;
#endif

    TyFHp  (rgTy, visym);
    SetSym (isymSave);
} /* TyFGlobal */


/***********************************************************************
 * T Y	 F   L O C A L
 *
 * Get type information for a local var with the given name.
 * Since isym is given, sbVar is of limited use...
 *
 * Some versions call routines which change visym, so they save it and
 * reset it afterwards.
 */

void TyFLocal (rgTy, sbVar, isym)
    pTYR	rgTy;		/* type info to get */
    char	*sbVar;		/* name to match    */
    long	isym;		/* current symbol   */
{
    long	isymSave = isym;

#ifdef INSTR
    vfStatProc[387]++;
#endif

    SetSym (isym);
    TyFHp  (rgTy, isym);
    SetSym (isymSave);
} /* TyFLocal */
#endif  /* HPSYMTABII */


/***********************************************************************
 * T Q	 F   T Y
 *
 * Hide details of TYR by returning the desired type qualifier.
 */

int TqFTy (ty, cnt)
    pTYR	ty;	/* type to check   */
    int		cnt;	/* which qualifier */
{

#ifdef INSTR
    vfStatProc[388]++;
#endif

    switch (cnt)
    {
	case 1: return (ty->td.tq1);
	case 2: return (ty->td.tq2);
	case 3: return (ty->td.tq3);
	case 4: return (ty->td.tq4);
	case 5: return (ty->td.tq5);
	case 6: return (ty->td.tq6);
    }

} /* TqFTy */


/***********************************************************************
 * A D J   T D
 *
 * Either "push" a new type qualifier (if given), or "pop" the top tq (if
 * none is given), to/from the given type descriptor.  Give an error if
 * there are already too many when trying to add one.
 *
 * For HPSYMTABs, we set/clear/advance the DNTT index to the corresponding
 * symbol table record.  It's only safe to do this when pushing or popping
 * a tq != tqNil (to avoid confusion with DXTYR use of XTYR).
 */

void AdjTd (ty, tq, isym)
    pTYR	ty;	/* type to adjust	  */
    int		tq;	/* tq to add if not tqNil */
    long	isym;	/* DNTT index of type info for tq -- HPSYMTABs only */
{
    register pTDR  td = & (ty->td);		/* type descriptor ptr */
    register pXTYR xty = (pXTYR) (ty + 1);	/* extension record */

#ifdef INSTR
    vfStatProc[389]++;
#endif

/*
 * PUSH NEW QUALIFIER:
 */

    if (tq != tqNil)
    {
	if (td->tq6 != tqNil)		/* all seats are taken */
	{
	    Panic ((nl_msg(569, "Type information overflow (more than 6 type qualifiers) (IE569)")));
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
	else if ((xty->isymTq != isymNil)	/* not nil symbol ptr	   */
	     AND (vlc != lcFortran))		/* not a FORTRAN array ptr */
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

int CbFTy (ty, fFullArray)
    pTYR	ty;		/* type to check	*/
    int		fFullArray;	/* full array, not bt?	*/
{
    register int itq;		/* index of current tq	*/
    register int tq;		/* current tq		*/
    pXTYR	xty = (pXTYR) (ty + 1);	/* exten record	*/
    long	isymSave = visym;       /* isym index to be restored */
    long	cb;		/* byte count		*/

#ifdef INSTR
    vfStatProc[390]++;
#endif

    if (ty == tyNil)
	return (CBINT);

    for (itq = 1; itq <= 6; itq++)
    {
	if ((tq = TqFTy (ty, itq)) == tqNil)	/* no more tq's */
	    break;

#ifdef S200
	if (tq == tqPtr)			/* a pointer */
	    return (CBPOINT);
	if (tq == tqFunc)			/* a function pointer */
	    if (vlc == lcPascal)
		return (2*CBPOINT);		/* 8-byte items in Pascal */
	    else
		return (CBPOINT);
#else
	if ((tq == tqPtr) OR (tq == tqFunc))	/* a pointer */
	    return (CBPOINT);
#endif

	if ((tq == tqArray)			/* an array	   */
	AND fFullArray				/* want full size  */
	AND (xty->isymTq != isymNil))		/* have isym index */
	{
		isymSave = visym;
		SetSym (xty->isymTq);
#ifdef HPSYMTAB
		cb = vsymCur->darray.bitlength / SZCHAR;
#else  /* HPSYMTABII */
                if (vsymCur->darray.arrayisbytes) {
		   cb = vsymCur->darray.arraylength;
                }
                else {
		   cb = vsymCur->darray.arraylength / SZCHAR;
                }
#endif /* HPSYMTABII */
		SetSym (isymSave);
		return (cb);
	}
    }
    if (ty->td.width != 0)			/* a bit field */
	return (-1);

    switch (ty->td.bt)				/* every bt should be here */
    {
	case btNil:	
                        if ((xty->st == stExtend) AND
                            (xty->isymRef != -1)) /* compute size of HP */
			{			  /* special type	*/
			    DNTTPOINTER dnttptr;
			    dnttptr.word = 0;
			    dnttptr.dnttp.extension = 1;
			    dnttptr.dnttp.index = xty->isymRef;
#ifdef SPECTRUM
			    return (CbFDntt (dnttptr, false));
#else
                            return (CbitsFDntt (dnttptr, false) / SZCHAR);
#endif
			}
			return (CBINT);		/* take a guess	*/

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

long IsymFNextTqSym (isym)
    long	isym;		/* DNTT index of type info for qual */
{
    DNTTPOINTER       nexttype;	/* ptr to next DNTT type rec	*/
    register KINDTYPE kind;	/* kind of next DNTT type rec	*/

#ifdef INSTR
    vfStatProc[391]++;
#endif

    SetSym (isym);	  	/* set to current tq sym */
    kind = vsymCur->dblock.kind;

    do {
	switch (kind)
	{
	    default:		sprintf (vsbMsgBuf,
				(nl_msg(570, "Internal Error (IE570) (%d, %d)")), kind, visym);
	    			Panic (vsbMsgBuf);

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

void DimFTy (tyArray, iTqPos, cbitsArray, lbArray, hbArray)
    pTYR	tyArray;	/* TYR record describing array		*/
    long	iTqPos;		/* relative tq count for which info is  */
				/*   desired:  + values = position from */
				/*   beginning of list (first = 1)	*/
				/*   - values = position from end of	*/
				/*   list (last item = -1)		*/
    long	*cbitsArray;	/* size in bits of array element	*/
    long	*lbArray;	/* lower bound of this dimension	*/
    long	*hbArray;	/* high bound of this dimension		*/
{
    pXTYR	xty = (pXTYR) (tyArray + 1);	/* quick pointer	*/
    DNTTPOINTER	dnttElem;			/* elem type DNTT ptr	*/
    DNTTPOINTER	dnttSub;			/* subscript DNTT ptr	*/
    long	isym;				/* index into DNTT	*/
    long	itq;				/* tq index		*/
    long	tq;				/* tq qualifier		*/
    long	arraySize;			/* size of entire array */

#ifdef INSTR
    vfStatProc[392]++;
#endif

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
		UError ((nl_msg(412, "FORTRAN variable not pure array")));
	}
	iTqPos += itq;				/* relative to start */
    }

/*
 * Set language-dependent defaults if there is no symbol information.
 * Note that this lets us index off simple variables and constants.
 * It's only safe to use xty if tq1 != tqNil (e.g. it's not a DXTYR).
 */

#ifdef HPSYMTABII
    if ((tyArray->td.bt == btNil) && (xty->btX == TX_COBOL)) {
       if (viDim-- == 0) { 
          char sbError[40];

          sprintf(sbError,
                  (nl_msg(294,"Too many array subscripts, expected %d")), 
                  viDimMac);
          UError(sbError);
       }
       *lbArray = vrgDims[viDim].low;
       *hbArray = vrgDims[viDim].high;
       *cbitsArray = vrgDims[viDim].bitsize;
	return;
    }
#endif

    if ((TqFTy (tyArray, 1) == tqNil)		/* no qualifiers	*/
    OR  (xty->st != stExtend)			/* no extension info	*/
    OR  ((isym = xty->isymTq) == isymNil))	/* no symbol info	*/
    {
	*lbArray = (vlc != lcC);		/* C starts 0, others at 1 */
	*hbArray = intMax;
	*cbitsArray = CbFTy (tyArray, false) * SZCHAR;
						/* no choice: use base size */
	return;
    }

/*
 * Find DNTT entry corresponding to the indicated tq modifier
 */

    SetSym (isym);

    for (itq = iTqPos - 1; itq > 0; itq--)	/* find specified DNTT	*/
    {						/* record from iTqPos	*/
	if ((isym = IsymFNextTqSym (isym)) == isymNil)
	    UError ((nl_msg(453, "Too many subscripts")));
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
#ifdef HPSYMTAB
	arraySize = vsymCur->darray.bitlength;
#else  /* HPSYMTABII */
	arraySize = vsymCur->darray.arraylength;
        *cbitsArray = vsymCur->darray.elemlength;
        if (vsymCur->darray.elemisbytes) {
           *cbitsArray *= SZCHAR;
        }
#endif /* HPSYMTABII */
	BoundsFDntt (dnttSub, lbArray, hbArray);  /* determine array bounds */
#ifdef HPSYMTABII
        if (*cbitsArray == 0) {
	   *cbitsArray = CbFDntt (dnttElem, true) * SZCHAR;
        }
#else
#ifdef FOCUS
        if ((*hbArray == intMax) || (vlc == lcPascal)) {
                                                  /* unknown bounds OR Pascal */
#else
        if (*hbArray == intMax) {                   /* unknown bounds */
#endif
            *cbitsArray = CbitsFDntt (dnttElem, true);
        }
        else {
            *cbitsArray = arraySize / (*hbArray - *lbArray + 1);
        }
#endif
    }
    else
	if ((vsymCur->dblock.kind == K_POINTER)
	AND (vlc == lcC)
	AND (TqFTy (tyArray, 1) == tqPtr))	/* supports [] of ptr in C */
	{
	    *lbArray = 0;
	    *hbArray = intMax;

	    dnttElem = vsymCur->dptr.pointsto;  /* get size of target object */
#ifdef SPECTRUM
	    *cbitsArray = CbFDntt (dnttElem, true) * SZCHAR;
#else
	    *cbitsArray = CbitsFDntt (dnttElem, true);
#endif
	}
        else			/* not ARRAY or valid POINTER DNTT entry */
	    UError ((nl_msg(453, "Too many subscripts")));

}  /* DimFTy */


#ifdef SPECTRUM
/****************************************************************************
 * C B   F   D N T T
 *
 * Given a DNTTPOINTER, return the size (in bytes) of the type it describes.
 */
long CbFDntt (dnttIn, fArray)
#else
/****************************************************************************
 * C B I T S   F   D N T T
 *
 * Given a DNTTPOINTER, return the size (in bits) of the type it describes.
 */
static long CbitsFDntt (dnttIn, fArray)
#endif
    DNTTPOINTER	dnttIn;		/* input DNTT type ptr; maybe immediate */
    FLAGT	fArray;		/* use full array size	*/
{
    long	isym;		/* DNTT index 			*/
    long	isymSave = visym;	/* DNTT index to be restored	*/
    KINDTYPE	kind;		/* kind of DNTT entry		*/
    DNTTPOINTER dnttPtr;	/* ptr to current DNTT entry	*/
    int		cb = -1;	/* result size			*/
#ifdef SPECTRUM
    ADRT	tempfp,tempap,temppc;
#endif

#ifdef INSTR
    vfStatProc[393]++;
#endif

    dnttPtr.word = dnttIn.word;

    do {
	if (dnttPtr.dnttp.immediate)
	{
#ifdef SPECTRUM
	    cb = dnttPtr.dntti.bitlength / SZCHAR;
#else
	    cb = dnttPtr.dntti.bitlength;
#endif
	    SetSym (isymSave);
	    return (cb);
	}

	isym = dnttPtr.dnttp.index;
	SetSym (isym);			/* now looking at chained type */
	kind = vsymCur->dblock.kind;

	switch (kind)
	{
	    default:		sprintf (vsbMsgBuf,
				    (nl_msg(571, "Internal Error (IE571) (%d, %d)")), kind, isym);
	    			Panic (vsbMsgBuf);

	    case K_FUNCTION:	dnttPtr = vsymCur->dfunc.retval;	break;
#ifdef HPSYMTABII
	    case K_FPARAM:	dnttPtr = vsymCur->dfparam.type;
				/* check for fortran character*(*) variable */
				if (dnttPtr.dntti.immediate &&
				   ((dnttPtr.dntti.type == 
                                     T_FTN_STRING_SPEC) ||
				   (dnttPtr.dntti.type == 
                                     T_FTN_STRING_S300_COMPAT)) &&
				   (dnttPtr.dntti.bitlength == 0)) {
                                   tempfp = vsp;
                                   tempap = vsp;
                                   temppc = vpc;
                                   NextFrame(&tempfp,&tempap,&temppc);
				   if (dnttPtr.dntti.type == T_FTN_STRING_SPEC){
                                      cb = SZCHAR * GetWord(tempfp + 
					        vsymCur->dfparam.location -
                                                4, spaceData);
                                   }
                                   else {
                                      cb = SZCHAR * GetWord(tempfp + 
					        vsymCur->dfparam.misc,
                                                spaceData);
                                   }
                                }
				break;
#else 
	    case K_FPARAM:	dnttPtr = vsymCur->dfparam.type;	break;
#endif
	    case K_SVAR:	dnttPtr = vsymCur->dsvar.type;		break;
#ifdef HPSYMTABII
	    case K_DVAR:	dnttPtr = vsymCur->ddvar.type;
				/* check for fortran character*(*) variable */
				if (dnttPtr.dntti.immediate &&
				   ((dnttPtr.dntti.type == 
                                     T_FTN_STRING_SPEC) ||
				   (dnttPtr.dntti.type == 
                                     T_FTN_STRING_S300_COMPAT)) &&
				   (dnttPtr.dntti.bitlength == 0)) {
                                      cb = SZCHAR * GetWord(vsp + 
					        vsymCur->ddvar.location +
                                                4, spaceData);
                                }
				break;
#else
	    case K_DVAR:	dnttPtr = vsymCur->ddvar.type;		break;
#endif
	    case K_CONST:	dnttPtr = vsymCur->dconst.type;		break;
	    case K_FIELD:	dnttPtr = vsymCur->dfield.type;		break;

	    case K_TYPEDEF:	dnttPtr = vsymCur->dtype.type;		break;
	    case K_TAGDEF:	dnttPtr = vsymCur->dtag.type;		break;

	    case K_POINTER:	cb = CBPOINT * SZCHAR;			break;
	    case K_ENUM:	cb = vsymCur->denum.bitlength;		break;
	    case K_SET:		cb = vsymCur->dset.bitlength;		break;
	    case K_SUBRANGE:	cb = vsymCur->dsubr.bitlength;		break;
	    case K_STRUCT:	cb = vsymCur->dstruct.bitlength;	break;
	    case K_UNION:	cb = vsymCur->dunion.bitlength;		break;
	    case K_FILE:	cb = vsymCur->dfile.bitlength;		break;
	    case K_FUNCTYPE:	cb = CBPOINT * SZCHAR;			break;
	    case K_ARRAY:	if (fArray)
#ifdef HPSYMTAB
	    			    cb = vsymCur->darray.bitlength;
#else  /* HPSYMTABII */
                                    if (vsymCur->darray.arrayisbytes) {
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
#endif /* HPSYMTABII */
				else
				    dnttPtr = vsymCur->darray.elemtype;
				break;
#ifdef HPSYMTABII
	    case K_COBSTRUCT:   cb = vsymCur->dcobstruct.bitlength;	break;
#endif
	}

    } while (cb < 0);

    SetSym (isymSave);		/* reset to isym value at entry	*/

#ifdef SPECTRUM
    return (cb / SZCHAR);
#else
    return (cb);
#endif

}  /* CbFDntt */
