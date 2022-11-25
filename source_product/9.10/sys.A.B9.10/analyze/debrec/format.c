/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/format.c,v $
 * $Revision: 1.2.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:22:46 $
 */

/*
 * Original version based on:
 * Revision 63.6  88/05/19  10:36:03  10:36:03  markm
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
 * These routines handle the analysis of format specifiers and the printing
 * of data values according to formats, including structure, enum, and type
 * dumps.
 */

#include <ctype.h>
#include <signal.h>
#include "cdb.h"

int	vcNest;			/* nest level for printing structs */
char	vsbFmt[20] = sbFmtDef;	/* format spec for addresses	   */
static	char	vsbAdr[20];		/* last address formatted	   */

FLAGT	vfFPECaught;		/* flag for detected FPE	  */

FLAGT vfJustChecking = false;  /* For printing "bad pointer" for     */
FLAGT vfBadAccess = false;     /* a string rather that "bad access", */
                               /* and quitting                       */


/***********************************************************************
 * NAMES OF BASIC TYPES (a mapping):
 */

static char *vmpBtSb[] = {
	"undefined",
	"function argument",
	"char",
	"short",
	"int",
	"long",
	"float",
	"double",
	"struct",
	"union",
	"enum",
	"member of enum",
	"unsigned char",
	"unsigned short",
	"unsigned int",
	"unsigned long",

	"integer*2",			/* FORTRAN forms: */
	"integer*4",
	"logical*1",
	"logical*2",
	"logical*4",
	"real*4",
	"real*8",

	"integer",			/* Pascal forms: */
	"unsigned integer",
	"real",
	"longreal",
	"record",

	"character",			/* T_STRINGHP (FORTRAN) */
	"string",			/* T_STRINGHP (Pascal)	 */
	"file-of-text",			/* T_TEXT		 */
	"format-label",			/* T_FLABEL		 */
	"set",				/* TX_SET		 */
#ifdef XDB
        "boolean",                      /* TX_BOOL               */
#endif
#ifdef HPSYMTABII
	"packed-decimal",		/* T_PACKED_DECIMAL	 */
#endif
};

#define	btInt2		16		/* names of indices above, see ty.h */
#define	btInt4		17
#define	btLog1		18		/* Note:  These are for local use  */
#define	btLog2		19		/*  only; cannot be stored into a  */
#define	btLog4		20		/*  ty->td.bt field (only 4 bits)  */
#define	btReal4		21
#define	btReal8		22

#define	btInteger	23
#define	btUInteger	24
#define	btReal		25
#define	btLongReal	26
#define	btRecord	27

#define	btStrHPF	28
#define	btStrHPP	29
#define	btText		30
#define	btFLabel	31
#define	btSet		32

#ifdef XDB
#define btBoolean       33
#ifdef HPSYMTABII
#define btPackedDec     34
#endif
#endif


/***********************************************************************
 * C A T C H    F P E
 *
 * Signal handler for catching FPE signal.
 * The caller must clear the vfFPECaught flag before setting up this
 * routine as the signal handler.
 */

void CatchFPE (sig,code,scp)
int sig;
int code;
struct sigcontext *scp;
{

#ifdef INSTR
    vfStatProc[121]++;
#endif

    vfFPECaught = true;	       /* set flag */
#ifdef SPECTRUM
    scp->sc_pcoq_head += 4;
    scp->sc_pcoq_tail += 4;
    scp->sc_sl.sl_ss.ss_frstat &= 0xffffffbf;
#endif
    signal (SIGFPE, SIG_IGN);  /* ignore any more til old handler restored */
}  /* CatchFPE */


/***********************************************************************
 * S B	 F   A D R
 *
 * Convert an address to a printable form using user-specified or default
 * format.  vsbFmt[] should never be nil (it is initialized).
 */

char * SbFAdr (adrLong, fMask)
    long	adrLong;		/* address to convert */
    FLAGT	fMask;			/* mask it to short?  */
{

#ifdef INSTR
    vfStatProc[122]++;
#endif

#if (CBINT == CBSHORT)
    if (fMask)				/* mask given	   */
	adrLong &= 0x0000ffffL;
#endif

    sprintf (vsbAdr, vsbFmt, adrLong);	/* hope it works! */
    return  (vsbAdr);

} /* SbFAdr */


/***********************************************************************
 * D F	 F   T Y
 *
 * Given a type, determine what display format to use for it.
 * This routine implements "alternate forms" for strings and structures.
 * Also, it always returns a valid df, never dfNil.
 */

DFE DfFTy (ty, fAlternate)
    pTYR	ty;			/* type to evaluate	  */
    FLAGT	fAlternate;		/* treat strings as such? */
{
    int		bt = ty->td.bt;		/* base type		*/
    int		tq = TqFTy (ty, 1);	/* curr type qualifier	*/

#ifdef INSTR
    vfStatProc[123]++;
#endif

/*
 * CHECK FOR ALTERNATE FORM:
 */

    if (fAlternate					/* alternate form OK */
    AND ((tq == tqPtr) OR (tq == tqArray))		/* pointer or array  */
    AND (TqFTy (ty, 2) == tqNil)			/* just * or []	     */
    AND (bt == btChar))					/* char* or char[]   */
    {
	return ((tq == tqPtr) ? dfPStr : 
		   ((vlc == lcPascal) ? dfPAC : dfStr));    /* use alternate */
    }

/*
 * CHECK FOR ARRAY, POINTER, OR FUNC:
 */

    if (tq != tqNil)					/* has a prefix  */
	return (dfAddr);				/* will show adr */

#ifdef FOCUS
/*
 * CHECK FOR REGISTER:
 */

    if (ty->td.st == stReg)
	return (dfHex);
#endif

/*
 * CHECK FOR EXTENDED TYPE:
 */
    if (bt == btNil)				/* also not btDouble */
    {
	pXTYR xty = (pXTYR) (ty + 1);		/* so this is safe */

	switch (xty->btX)
	{
	    case T_STRING200:	  return (dfString200);
#ifdef HPSYMTAB
	    case T_LONGSTRING200: return (dfLongString200);
#endif
	    case T_STRING500:	  return (dfString500);
	    case T_TEXT:	  return (dfText);
	    case T_FLABEL:	  return (dfFLabel);
	    case TX_SET:	  return (dfSet);
#ifdef HPSYMTABII
	    case T_FTN_STRING_S300_COMPAT:
	    case T_FTN_STRING_SPEC:	return (dfStringFtnSpec);
	    case T_MOD_STRING_SPEC:	return (dfStringModSpec);
	    case T_PACKED_DECIMAL:	return (dfPackedDecimal);
	    case TX_COBOL:		return (dfCobstruct);
#endif	/* HPSYMTABII */
	}
    }						/* else fall through */
/*
 * CHECK BASIC TYPES:
 */

    switch (bt)
    {
	case btUnion:
	case btStruct:	return (fAlternate ? dfStruct : dfAddr);

	case btUChar:
	case btUShort:
	case btUInt:
	case btULong: {
#ifdef XDB
	                 pXTYR xty = (pXTYR) (ty + 1);
	                 if (((xty->btX == TX_BOOL) ||
	                      (xty->btX == TX_BOOL_S300) ||
	                      (xty->btX == TX_BOOL_VAX)) &&
                             (vlc != lcC)) {
                            return (dfBool);
                         }
#endif
	                 return (dfUnsigned);
                      }

	default:				/* note well */
	case btNil:
	case btShort:
	case btInt:
	case btFArg:
	case btEMember:
	case btLong:	return (dfDecimal);

	case btFloat:
	case btDouble:	return (dfGFloat);

	case btEType:	return (dfEnum);

#ifdef XDB /* don't understand what non-xdb case does */
	case btChar:	return (dfChar);
#else
        case btChar:    return ((fAlternate && (vlc == lcFortran)) ?
                                 dfDecimal : dfChar);
#endif

    } /* switch */

} /* DfFTy */



/***********************************************************************
 * S E T U P   S T R I N G   H P
 *
 * Do special-case handling of HPSYMTAB(II) strings for display formats
 * dfPStr, dfStr, dfString500, and dfString200.  Only called if the basetype
 * or the combination of language and given display format require
 * dfPStr, dfStr, and dfString500.  Only called if the basetype or the
 * combination of language and given display format require
 * interpretation of the object as an HP string.
 *
 * Modify adrLong and len carefully for the first loop (cPrint == 0).
 * For Series 500 this is based on either a FOCUS string marker (usually
 * from FORTRAN) or a Pascal-type string (4-byte length word followed by
 * chars).  Series 200 FORTRAN CHAR* variables are simply a sequence
 * of characters and Pascal string variables have a 1-byte length word
 * followed by chars.
 *
 * For later loops (cPrint > 0), use the previous adrLong and len for
 * string markers (don't change anything), or use the next length word
 * for Pascal strings (round up to the next word if necessary -- 500 only).
 *
 * Also set mode->len for later calls of DispVal() using vmode and
 * IncFTyMode(), either to the size of a string marker, or subtract the
 * size of the actual Pascal string (rounded up to words on 500, including the
 * length word, so we have the total for all strings displayed).  This
 * is a "hidden" side effect.
 */

void SetupStringHP (ty, mode, df, cPrint, padrLong, plen, plenActual)
    pTYR	ty;		/* underlying type info */
    pMODER	mode;		/* mode info to modify	*/
    DFE		df;		/* given display format	*/
    int		cPrint;		/* how many printed yet	*/
    long	*padrLong;	/* adr to modify	*/
    int		*plen;		/* maximum length of string */
    int		*plenActual;	/* length of string to print */
{
    long	adrLong = *padrLong;	/* quick value	*/
    long	adrTemp;		/* temp  value	*/
    pXTYR	xty = (pXTYR) (ty + 1);

#ifdef INSTR
    vfStatProc[128]++;
#endif

#ifdef FOCUS
    if ( (df == dfPStr)				/* force use as STRING MARKER */
    OR  ((df != dfStr) AND (vlc == lcFortran)))	/* defaults to string marker  */
    {
	if (cPrint == 0)			/* first time only */
	{
	    adrLong &= (~3);			/* force to start of word */

	    if (adrTemp  = GetWord (adrLong + 4, spaceData))   /* ptr != null */
		adrTemp += GetWord (adrLong + 8, spaceData) + 3 + xty->cb;
				/* add index and offset  */

	    *plen     = GetWord (adrLong + 12, spaceData);	/* get length */
	    *padrLong = adrTemp;
	    *plenActual = *plen;		/* actual = max for FORTRAN */

	    mode->cnt = 1;			/* for IncFTyMode() */
	    mode->len = 4 * CBLONG;
	}
    }
    else					/* PASCAL STRING */
    {
	if (cPrint == 0)			/* first time only	  */
	    adrLong &= (~3);			/* force to start of word */
	else
	    /* force to NEXT word if not at start of word */
	    adrLong = (adrLong + 3) & (~3);

	*plenActual = GetWord (adrLong, spaceData);
	*padrLong   = adrLong + ((cPrint < 0) ? 0 : CBLONG);
	*plen = CbFTy (ty, false) - 4;

	/* assumes it was set to zero for decrement here */
	mode->len -= (((*plen + CBLONG - 1) / CBLONG) + 1) * CBLONG;
    }
#endif /* FOCUS */

#if (S200 || SPECTRUM)
    if (vlc == lcFortran)			/* FORTRAN CHAR* variable */
    {
	*plen = CbFTy (ty, false);
	*plenActual = *plen;
	mode->cnt = 1;				/* for IncFTyMode() */
	mode->len = *plen;
    }
    else					/* PASCAL STRING */
    {
#ifdef S200
        if (df == dfLongString200) {
            *plen       = CbFTy (ty, false) - 4;
            GetBlock(adrLong, spaceData, plenActual, 4);
            *padrLong   = adrLong + 4;
        } else {
            *plen       = CbFTy (ty, false) - 1;
            *plenActual = GetByte (adrLong, spaceData);
            *padrLong   = adrLong + 1;
        };
#else 	/* SPECTRUM */
	*plen       = CbFTy (ty, false) - 4;
	*plenActual = GetWord (adrLong, spaceData);
	*padrLong   = adrLong + 4;
#endif /* SPECTRUM */

	/* assumes it was set to zero for decrement here */
	mode->len -= *plen;
    }
#endif /* S200 or SPECTRUM */

} /* SetupStringHP */


/***********************************************************************
 * D I S P   V A L
 *
 * Given address, type, and maybe format information, print data values.
 * Does a lot of arcane work first to figure out what df and len to use,
 * the column width of each field, how to mask the data before printing
 * it, etc.  There is an explanation of this in the theory of operation.
 *
 * Has support for len == 3, but only uses the first CBLONG bytes if
 * len > CBLONG.  (Address increments by len, however.)
 *
 * Prints trailing newlines only as needed before starting new lines
 * (leaves the last line unfinished, as some callers require, except for
 * dfProc, which unfortunately always has the line finished).
 *
 * If not given a mode record, it uses vmode (the global mode), which is
 * always updated in any case.  This records the mode when used, even if
 * it's figured on the fly from type info, for possible later use.
 */

void DispVal (adrLong, tyIn, mode, fPadName, fIndirect, fAlternate)
    long	adrLong;		/* address of data to show	*/
    pTYR	tyIn;			/* data type info, if any	*/
    pMODER	mode;			/* print mode info, if any	*/
    FLAGT	fPadName;		/* pad value name with blanks?	*/
    FLAGT	fIndirect;		/* indirect via adr, or direct? */
    FLAGT	fAlternate;		/* alternate form OK?		*/
{
    int		count;			/* how many objects to show	*/
    DFE		df;			/* display format to use	*/
    int		len;			/* size of one object (bytes)	*/
    long	lenActual;		/* size of string object	*/
    int		bitsize;		/* size of one object (bits)	*/
    int		cbTy;			/* results of CbFTy()		*/
    int		btNew	   = btNil;	/* to change bt for "." (dot)	*/
    FLAGT	fRandomLen = false;	/* random size object?		*/
    FLAGT	fSingle;		/* printing just one object?	*/

    int		width;			/* of field for some printf()s	*/
    int		precision;		/* of field for some printf()s	*/
    int		modval = 1;		/* objects per line		*/
    int		cPrint = 0;		/* objects printed so far	*/
    int		isymsave;		/* to save visym, it can change */

    char	*sbVar;			/* name of object		*/
    STUFFU	stuff;			/* value to show, flexible form	*/
    long	val;			/* value or address to show	*/
    long	valMask;		/* value after masking		*/

    ADRT	adrStr;			/* address of string value	*/
    int		cbStr;			/* length of string		*/
    int		lenStr;			/* maximum to display		*/
    char	*sbQuote;		/* how to quote a string	*/
    FLAGT	fStringHP = false;	/* special HPSYMTAB(II) string?	*/

    long	(*pSigFPE) ();		/* previous FPE signal handler	*/
    void	CatchFPE ();		/* new FPE signal handler	*/
    STUFFU	stuff1;			/* dummy area for work		*/

    char        buf[80];		/* to contain disassembly 	*/
    int		ivalTySave;		/* to save tyIn->valTy field	*/
					/* if it is to be modified	*/
    pXTYR	xty = (pXTYR) (tyIn + 1);

#ifdef INSTR
    vfStatProc[129]++;
#endif

    isymsave = visym;

    if ((cbTy = CbFTy (tyIn, false)) < 1)  /* base type size for HPSYMTABs */
	 cbTy = CBINT;	 /* don't care if it's bit field or has zero size */

/*
 *  SAVE valTy IF NEEDED
 *
 *  If the item we are printing is a bit field, and we are printing
 *  more than one, then the valTy field (which is the bit offset within
 *  the byte pointed to by adrLong) will be changed to address the next
 *  bit field.  adrLong is only changed locally, however the valTy field
 *  in tyIn must be saved, so it can be restored to its original value
 *  on exit from Dispval.
 */

    if (tyIn->td.width) {
       ivalTySave = tyIn->valTy;
    }

/*
 * SEE IF MODE INFO GIVEN:
 */

    if (mode == modeNil)		/* MODE NOT GIVEN, will use ty info */
    {
	mode  = vmode;			/* do it in global */
	count = 1;			/* just show one   */
	df    = dfNil;			/* no format yet   */
	len   = 0;			/* gets set later  */
    }
    else				/* USE GIVEN MODE INFO */
    {
	count = mode->cnt;		/* always valid			*/
	df    = mode->df;		/* might be dfNil ("/n" format)	*/
	len   = mode->len;		/* always valid; maybe < 0	*/

	fStringHP = ((vlc == lcFortran) OR (vlc == lcPascal))
		 AND ((df  == dfPStr)	 OR (df  == dfStr));

/*
 * FORCE THE TYPE INFO TO THE RIGHT SIZE:
 *
 * Only done when given both a mode and a length of CBCHAR - CBLONG (either
 * the user gave them explicitly or vmode was used and had such a length).
 * Sets the new type based first on the new size, only then on the df.
 * This way, when given "sbWord/c2;<cr>;<cr>;.='H'", we do the "correct"
 * character assignment (in this case, two bytes!).  Unfortunately, we don't
 * have types for sizes other than CBCHAR, CBSHORT, CBLONG, and CBFLOAT.
 * Do NOT change the type if len > CBLONG; let other code handle assigns to
 * such objects.
 * 
 * The new type (btNew) is set here but not put into tyIn until AFTER DispVal()
 * is done displaying, so ValFAdrTy() gets current (correct) type information
 * it needs.  The object name is tossed immediately if the new length differs
 * from the old one.  Other values are always saved.
 */

	if ((len >= CBCHAR) AND (len <= CBLONG)
        AND (tyIn->td.bt != btStruct) AND (tyIn->td.bt != btUnion)
#ifdef FOCUS
	AND ((tyIn->td.bt != btNil) OR (xty->btX != T_STRING500)))
#endif
#ifdef S200
	AND ((tyIn->td.bt != btNil) OR (xty->btX != T_STRING200) OR
        (xty->btX == T_LONGSTRING200)))
#endif
#ifdef SPECTRUM
        AND (tyIn->td.bt != btEType)
	AND ((tyIn->td.bt != btNil) OR (xty->btX != T_FTN_STRING_SPEC)
        OR (xty->btX != T_MOD_STRING_SPEC)))
	/* may make changes here for Spectrum strings */
#endif
	{
	    switch (len)
	    {
	    case CBCHAR:
			btNew = (df == dfUnsigned) ? btUChar : btChar;
			break;

	    case CBSHORT:
			btNew = (df == dfUnsigned) ? btUShort : btShort;

#if (CBSHORT == CBFLOAT)
			if ((df == dfEFloat)
			OR  (df == dfFFloat)
			OR  (df == dfGFloat))
			{
			    btNew = btFloat;
			}
#endif
			break;

	    default:	btNew = (df == dfUnsigned) ? btULong : btLong;

#if (CBLONG == CBFLOAT)
			if ((df == dfEFloat)
			OR  (df == dfFFloat)
			OR  (df == dfGFloat))
			{
			    btNew = btFloat;
			}
#endif
			break;
	    }
	    /* note that btNew is not used till later */

	    if (len != cbTy)			/* change of length */
	    {
		tyIn->sbVar = -1;		/* invalidate name of object */
		cbTy = len;			/* update "size of type" */
	    }
	} /* if */
    } /* else */

/*
 * SET FORMAT AND LENGTH:
 *
 * After this we always have a usable df and valid (positive) len.  If the
 * object is an array or pointer, or in some cases a struct, DfFTy() wants
 * us to use dfAddr, so we modify len accordingly.  (CbFTy() already
 * returned CBPOINT where dfPStr might apply.)  Also set fRandomLen if the
 * object is random-length (dfStr or dfPStr with no length given, or if
 * len < 0, meaning last display was random length, probably a string).  
 */

    if (df == dfNil)			/* NO DF GIVEN, or "/n" used */
    {
	df  = DfFTy (tyIn, fAlternate);
	if (df == dfPAC)
	    len = CbFTy (tyIn, true);
	else
	    len = (df == dfAddr) ? CBPOINT : cbTy;
	fRandomLen = ((df == dfStr) OR (df == dfPStr));
    }
    else if (len < 0) 			/* DF WAS GIVEN, but len is random */
    {
	if ((len == -1) AND (cbTy > CBLONG))	/* probably need to shorten */
	    len  = CBLONG;
	else
	    len  = cbTy;

	/* note that dfStr is handled specially later, anyway; this is */
	/*   relevant if vmode was used and previous string was 1 char */

	fRandomLen = true;
    }
    else {
       if ((len == CBPOINT) && (df == dfPStr)) {
	  fRandomLen = true;
       }
    }
    /* else if df and len both given, use the given len */

#ifdef FOCUS
    fStringHP |= (df == dfString500); 	/* check if set due to type */
#endif
#ifdef S200
    fStringHP |= (df == dfString200) 	/* check if set due to type */
                 || (df == dfLongString200);
#endif
#ifdef SPECTRUM
    fStringHP |= ((df == dfStringModSpec) OR (df == dfStringFtnSpec)); 	
                                        /* check if set due to type */
#endif

/*
 * SAVE NET VALUES FOR LATER CALLS:
 */

     mode->cnt = count;
     mode->df  = df;
     mode->len = (df == dfPStr) ? CBPOINT : len;

/*
 * SET OBJECTS PER LINE, COLUMNS PER OBJECT, etc:
 *
 * Set modval (defaults to 1), bitsize, width, and precision.  Also modify
 * len and some flags for some formats.  Not all formats use len, width,
 * bitsize, and precision, but all use modval and the flags.
 */

    bitsize = SZCHAR * Min (len, CBLONG);
    
    switch (df)					/* default: no action */
    {
	case dfHex:	/* fall through */
	case dfOctal:	{   /* precision (digits) based on bits per digit */
			    int	 bpd  = (df == dfOctal) ? 3 : 4;
			    precision = (bitsize + bpd - 1) / bpd;
			}
			/* fall through */
	case dfAddr:	/* fall through */
	case dfUnsigned:/* fall through */
	case dfDecimal:	/* fall through */
	case dfChar:	/* precision is hardwired in later */

			modval = (len < 3) ? 8 : 4;	/* nice round numbers */

			switch (len)	/* set width to largest needed */
			{
			    case  1:	width =  4;	break;
			    case  2:	width =  7;	break;
			    case  3:	width =  9;	break;
			    default:	width = 12;	break;
			}
			break;

	case dfEFloat:	/* fall through */
	case dfFFloat:	/* fall through */
	case dfGFloat:
			if (len > CBFLOAT)
			{
			    modval =  2;
			    width  = 24;
			}
			else
			{
			    modval =  4;
			    width  = 14;
			}
			break;

	case dfType:	/* fall through	*/
	case dfProc:	fIndirect = false;	/* don't look up data */
			/* this also prevents printing of the name!   */
			break;

	case dfStr:	len = CBCHAR;		/* simple string */
			if (fRandomLen)
			    mode->len = 0;	/* init for later decrement */
			break;

	case dfString200:
	case dfLongString200:
	case dfString500:
#ifdef HPSYMTABII
	case dfStringFtnSpec:
	case dfStringModSpec:
#endif
#ifdef XDB
	case dfPackedDecimal:
#endif
			mode->len = 0;		/* init for later decrement */
			break;

        case dfInst:
                        mode->len = 0;
                        mode->cnt = 1;
                        modval = 1;             /* one per line */
                        btNew = btNil;
                        break;


    } /* switch */

    if (fSingle	= (count == 1))		/* just printing one */
    {
	width	  = 0;			/* use minimum width  */
	precision = 1;			/* at least one digit */
    }

/*
 * START TO PRINT EACH OBJECT; GET THE VALUE:
 *
 * Note that ValFAdrTy() does sign extension if required but does not
 * promote float to double or anything strange like that.
 */

    while (true)			/* quits when count zero */
    {

#ifndef FOCUS
	vimap = mode->imap;		/* sorta kludgey - sorry */
#endif

	if (fIndirect AND (df != dfStruct)	/* get data using address */
        AND (df != dfInst) 
#ifdef HPSYMTABII
	AND (df != dfCobstruct)
#endif
        )
	{
	    /* use len, not base size; do not cast to long */
	    ValFAdrTy (& stuff, adrLong, tyIn, len, false);
	    val = stuff.lng;			/* but leave stuff as is */

	    if (len == 3)			/* special, see ValFAdrTy() */
		val >>= SZCHAR;
	}
	else					/* just use address itself */
	    val = adrLong;

#ifndef FOCUS
	vimap = 0;
#endif

/*
 * Handle T_STRINGHP base types:  Set correct adr and len, and maybe modify
 * mode->cnt and mode->len for later calls of DispVal().  Do it here BEFORE
 * showing the address.
 */
	if (fStringHP)
	{
	    lenActual = len;
	    SetupStringHP (tyIn, mode, df, cPrint, & adrLong, & len,
			   & lenActual);
	    fRandomLen = false;			/* len was set here */
	}

/*
 * SHOW THE NAME OR ADDRESS if we can and if the time is right:
 */

	if (fIndirect			/* it's meaningful to show */
	AND (cPrint % modval == 0)	/* at start of a new line  */
        AND (df != dfInst))             /* at not disassembling    */
	{
	    if (fSingle				/* just one object	*/
	    AND (sbVar	 = SbInCore (tyIn[0].sbVar))  /* it has a name	*/
	    AND (*sbVar != chNull)		/* and it's valid	*/
	    AND (*sbVar != '*'))		/* doesn't start as "*"	*/
	    {					/* PRINT THE NAME	*/
#ifdef S200
                if (*sbVar != '@')		/* FORTRAN unnamed struct? */
#endif
		printf (fPadName ? "%-9s = "   : "%s = ",   sbVar);
	    }
	    else if ((! tyIn->fAdrIsVal)		/* val not in adr fld */
		 AND ((TqFTy (tyIn, 1) != tqArray)	/* not array either */
		   OR (df == dfPAC)))
	    {
		printf ("%s  ", SbFAdr (adrLong, true)); /* PRINT ADDRESS */
	    }
	}

/*
 * NOW SHOW THE VALUE:
 */

	valMask	= val & (Mask (bitsize));
	adrStr	= adrLong;		/* default for dfStr and dfString500 */

	switch (df)
	{
	    default:		sprintf (vsbMsgBuf,
				    (nl_msg(520, "Internal Error (IE520) (%d)")),
				    df);
				Panic (vsbMsgBuf);

	    case dfHex:		printf ("%#*.*lx", width, precision, valMask);
				break;
	    case dfOctal:	printf ("%#*.*lo", width, precision, valMask);
				break;
	    case dfAddr:	printf ("%*s",	   width, SbFAdr (val, true));
				break;
	    case dfUnsigned:	
                                /* special case $lang to display symbolically */
                        	if (FSbCmp (sbVar, "$lang"))
                                {
                                    switch (val)
                                    {
                                     default:        /* fall through */
                                     case lcC:        printf ("C");       break;
                                     case lcPascal:   printf ("Pascal");  break;
                                     case lcFortran:  printf ("FORTRAN"); break;
#ifdef HPSYMTABII
                                     case lcCobol:    printf ("Cobol");   break;
#endif
                                    }
                                 
                                    if (vlcSet == lcDefault)
                                        printf ((nl_msg(91, " (default) ")));
                                }
                                else
				    printf ("%*lu",    width, valMask);
				break;
	    case dfDecimal:
	    			printf ("%*ld",    width, val);
				break;

	    case dfChar:
		if (fSingle) 			/* just one; width irrelevant */
		{
		    if (valMask < ' ')			/* control and octal */
			printf ("'^%c' (\\%#lo)", valMask + 0100, valMask);
		    else if (valMask > 0176)		/* just octal */
			printf ("'\\%#lo'", valMask);
		    else				/* just quoted char */
			printf ("'%c'", valMask);
		}
		else				/* many chars: minimize junk */
		{				/* effective width == 6	cols */
		    printf (((valMask <= ' ') OR (valMask > 0176)) ?
			    "\\%#4.4lo" : "    %c", valMask);
		}
		break;

	    case dfEFloat:
	    case dfFFloat:
	    case dfGFloat:
		vfFPECaught = false;		/* reset flag		*/
		pSigFPE = signal (SIGFPE, CatchFPE);  /* install new catcher */
		if (len <= CBFLOAT)
		    stuff1.fl = stuff.fl*1.0;	/* exercise as float	*/
		else
		    stuff1.doub = stuff.doub*1.0;  /* exercise as double */
		signal (SIGFPE, pSigFPE);	/* restore old handler	*/
		if (vfFPECaught)
		    printf ((nl_msg(92, "(not a real number)")));
		else
#ifdef SPECTRUM
/*
 * The following is required on spectrum because if printf is called 
 * directly, and there is a record file, the fprintf for the record
 * file will split the float in two, half in r23 and half on the stack,
 * thus getting incorrect results. The correct solution is to make printf
 * a varargs procedure, but that would require too many changes
 */
		    if (len <= CBFLOAT)
			sprintf (buf,
                                (df == dfEFloat) ? "%*e" :
				(df == dfFFloat) ? "%*f" : "%*g",
				width, stuff.fl);
		    else
			sprintf (buf,
                                (df == dfEFloat) ? "%*.14e" :
				(df == dfFFloat) ? "%*f" : "%*.15g",
			        width, stuff.doub);
                    printf(buf);
#else
		    if (len <= CBFLOAT)
			printf ((df == dfEFloat) ? "%*e" :
				(df == dfFFloat) ? "%*f" : "%*g",
				width, stuff.fl);
		    else
			printf ((df == dfEFloat) ? "%*.14e" :
				(df == dfFFloat) ? "%*f" : "%*.15g",
			        width, stuff.doub);
#endif
		break;

	    case dfType:	PxTy	     (tyIn);			break;
	    case dfEnum:        PxEnum 	     (val, tyIn); 		break;
	    case dfStruct:	IssFPxStruct (adrLong, tyIn, true);	break;
	    case dfProc:	PrintPos     (val, fmtFile + fmtProc + fmtSave);
									break;
	    case dfPStr:
		adrStr = val;		/* do indirection, fall through */

	    case dfStr:			/* fall through */

	    case dfPAC:			/* fall through */

	    case dfString200:
	    case dfLongString200:
	    case dfString500:
#ifdef HPSYMTABII
	    case dfStringFtnSpec:
	    case dfStringModSpec:
#endif
		if (fStringHP)
		    adrStr = adrLong;		/* use correct address */

		if (adrStr == 0)
		{
		    printf ((nl_msg(93, "<null pointer>")));
		    break;				/* stop now */
		}
                vfJustChecking = true;
                vfBadAccess = false;
                val = GetByte (adrStr, spaceData);
                vfJustChecking = false;
                if (vfBadAccess) {
		    printf ((nl_msg(275, "<bad pointer>")));
		    break;				/* stop now */
                }

		sbQuote = ((vlc == lcC) ? "\"" : "'");
		printf (sbQuote);			/* open quote */

		lenStr = fRandomLen ? 1024 : (fStringHP ? lenActual : len);
							/* maximum or actual */

		for (cbStr = 0; cbStr < lenStr; cbStr++)
		{
		    val = Mask (SZCHAR) & GetByte (adrStr + cbStr, spaceData);

		    if (fRandomLen AND (val == chNull))	/* end of string */
			break;

		    printf (((val < ' ') OR (val > 0176)) ?
			    "\\%#lo" : "%c", val);
		}

		printf (sbQuote);			/* close quote */
		break;

	    case dfText:	printf ((nl_msg(94,"{file-of-text}")));	break;
	    case dfFLabel:	printf ((nl_msg(95,"{format-label}")));	break;
	    case dfSet:		PxSet (tyIn, adrLong);	 	        break;
#ifdef SPECTRUM
#ifdef NOTDEF
	    case dfInst:	iDasm(val, 0, 0, buf); /* disassemble */
#else
	    case dfInst:	
                                printf("INSTRUCTION_JOH???\n");
#endif
                                mode->len += CBLONG;
#else
            case dfInst:        mode->len += (len = PrintInst (adrLong));
#endif
                                break;
#ifdef HPSYMTABII
#ifdef NOTDEF
            case dfCobstruct: 
                            PxCobstruct(adrLong, cbTy, xty->isymRef);
                            break;
#endif

            case dfBool: {
                            FLAGT istrue = 0;
                            int   xbt    = xty->btX;

                            switch (xbt) {
                               case TX_BOOL:
                                  istrue = val & 1;
                                  break;
                               case TX_BOOL_S300:
                                  istrue = val;
                                  break;
                               case TX_BOOL_VAX:
                                  istrue = val & 1;
                                  break;
                            }

                            if (istrue) {
                               if (vlc == lcFortran) {
                                  printf(".TRUE.");
                               }
                               else {
                                  printf("true");
                               }
                            }
                            else {
                               if (vlc == lcFortran) {
                                  printf(".FALSE.");
                               }
                               else {
                                  printf("false");
                               }
                            }
                            break;
                         }

#endif
#if (XDB && HPSYMTABII)
	    case dfPackedDecimal:	printf ((nl_msg(96,"{packed-decimal}")));	break;
#endif

	} /* switch */

/*
 * NOTE ACTUAL SIZE for random size object:
 */

	if (fRandomLen)
	{
	    cbStr += 1;			/* move past chNull */

	    if (df == dfStr)		/* remember size for IncFTyMode()  */
		mode->len -= cbStr;	/* HPSYMTABs strings done elsewhere */
	}

/*
 * QUIT LOOP OR ADVANCE TO THE NEXT OBJECT TO SHOW:
 */

	if (--count <= 0)		/* no more to do */
	    break;

	tyIn->sbVar = -1;		/* invalidate name of named object */

#ifdef SPECTRUM	/* actually I think this is always correct */
        if (fStringHP) {
           adrLong += len;
        }
        else if (df == dfStr) {
           adrLong += cbStr;
        }
        else if (df == dfPStr) {
           adrLong += CBPOINT;
        }
        else if (tyIn->td.width) {
           tyIn->valTy += tyIn->td.width;
           adrLong += (tyIn->valTy / SZCHAR);
           tyIn->valTy %= SZCHAR;
        }
        else {
           adrLong += len;
        }
#else
	adrLong += fStringHP ? len :
		   ((df == dfStr) ? cbStr :
		   ((df == dfPStr) ? CBPOINT : len));
#endif

/*
 * FINISH LINE OR APPEND BLANK:
 */

	if (df != dfProc)			/* not already done */
	{
	    if ((++cPrint % modval) == 0)	/* line is "full" */
		printf ("\n");
	    else
		printf (" ");
	}

    } /* while */

/*
 * SET NEW BASETYPE FOR "." (DOT):
 */

    if (btNew != btNil)
    {
	tyIn->td.bt  = btNew;
	tyIn->td.tq1 = tqNil;			/* have to clear this too */
    }

    if (tyIn->td.width) { 			/* restore original value */
       tyIn->valTy = ivalTySave;
    }

    visym = isymsave;

} /* DispVal */


/***********************************************************************
 * P X	 E N U M
 *
 * Print a "picture" of an enumeration member (print its name), given
 * the value and the type info for the enumeration.  Only called by
 * DispVal() to look up the name of a member from its value.  The caller
 * must finish the line (print the newline).
 */

void PxEnum (val, ty)
    long	val;
    pTYR	ty;
{

/*
 * Given a K_ENUM type, search its K_MEMENUMs for a member with a matching
 * value and if found, print its name.  Resets current symbol when done.
 */

    long	isymSave = visym;		/* to reset later	*/
    pXTYR	xty = (pXTYR) (ty + 1);		/* safe -- not btDouble	*/
    DNTTPOINTER nextmem;			/* next member of enum	*/

#ifdef INSTR
    vfStatProc[130]++;
#endif

    SetSym (xty->isymRef);			/* set to K_ENUM */
    nextmem = vsymCur->denum.firstmem;		/* first member */

    while (nextmem.word != DNTTNIL)		/* not nil pointer */
    {
	SetSym (nextmem.dnttp.index);		/* set to K_MEMENUM */
	if (val == vsymCur->dmember.value)	/* a hit */
	{
	    printf ("%s", SbInCore (vsymCur->dmember.name));
	    SetSym (isymSave);
	    return;
	}
	nextmem = vsymCur->dmember.nextmem;	/* no hit, try next */
    }
    printf ((nl_msg(97, "%d (value not defined for \"enum %s\")")),
	val, SbInCore (ty->sbVar));

    SetSym (isymSave);
    return;

} /* PxEnum */


/***********************************************************************
 * I S S   F   P X   S T R U C T
 *
 * Print a "picture" of a structure and its fields, optionally with
 * values.  Called by DispVal() and PxTy() for simple (non-array,
 * non-pointer) structure/union dumps, and calls one of them in turn
 * for each field (with or without data shown).  (The non-HPSYMTAB(II)
 * version still calls itself directly for simple nested structures.)
 *
 * The caller must provide adr and tyStruct for a btStruct or btUnion,
 * must print "<name> =" first (or "<name>" after) for the tyStruct if
 * the name is desired, and must finish the last line (e.g. ";\n").
 */

void IssFPxStruct (adr, tyStruct, fDoVal)
    ADRT	adr;		/* adr of structure */
    pTYR	tyStruct;	/* type information */
    FLAGT	fDoVal;		/* show values?	    */
{
    char	*sbIndent = "    ";	/* indent format */

/*
 * We do the printing, but don't return an iss value.
 * Resets current symbol when done.
 */

    long	isymSave = visym;		/* to reset later	*/
    FLAGT	fStruct;			/* struct, not union?	*/
    int		cNest;				/* nesting depth	*/
    pXTYR	xty = (pXTYR) (tyStruct + 1);	/* safe -- not btDouble */
#ifdef S200
    char       *sbVar;				/* to look at name      */
    FLAGT	fMap;				/* a FORTRAN map?       */
#endif

#ifdef INSTR
    vfStatProc[131]++;
#endif

/*
 * Make sure this is a legitimate request
 */
    if (((tyStruct->td.bt != btStruct) AND
	 (tyStruct->td.bt != btUnion)) OR 
	(tyStruct->td.tq1 != tqNil))
    {
	if (vlc == lcPascal)
	    UError ((nl_msg(429, "This does not appear to be a record or union")));
	else
	    UError ((nl_msg(430, "This does not appear to be a struct or union")));
    }

/*
 * PRINT START OF STRUCTURE:
 *
 * We don't have C tagnames easily available so we don't print them.
 */

    SetSym (xty->isymRef);				/* to struct or union */
    fStruct   = (vsymCur->dblock.kind == K_STRUCT);	/* which kind is it?  */
#ifdef S200
    fMap = false;
#endif

    if (vlc == lcPascal)				/* print start of it */
	printf (fStruct ? "record\n" : "union\n");
    else						/* assume it's C */
#ifdef S200
    if (vlc == lcFortran) {
	if (fStruct) {
	    sbVar = SbInCore(tyStruct->sbVar);
	    fMap = *sbVar == '@';
	}
	printf ("%s\n", fMap ? "map" : (fStruct ? "structure" : "union"));
    } else
#endif
	printf ("%s {\n", fStruct ? "struct" : "union");

    vcNest++;			/* up nesting level for indent printing */

/*
 * PRINT ALL FIELDS AND VARIANTS
 */

    IssFPxStrFields (adr, tyStruct, fDoVal, fStruct);

/*
 * PRINT CLOSE OF STRUCTURE:
 */

    vcNest--;

    for (cNest = 0; cNest < vcNest; cNest++)
	printf (sbIndent);

#ifdef SPECTRUM
    printf ((vlc == lcPascal) ? "end" : "}");
#else
    if (vlc == lcPascal)
	printf ("end");
    else if (vlc == lcFortran)
	printf ("end %s", fMap ? "map" : (fStruct ? "structure" : "union"));
    else
	printf ("}");
#endif

    SetSym (isymSave);
    return;

} /* IssFPxStruct */


/***********************************************************************
 * P X	 T Y
 *
 * Print a "picture" of type information.
 *
 * Called from various places, including IssFPxStruct() to print field
 * type info.  Calls IssFPxStruct() in turn for simple (non-array,
 * non-pointer) structures/unions.
 *
 * The caller must finish the last line (e.g. ";\n").
 */

void PxTy (ty)
    pTYR	ty;			/* type to show */
{
    int		tq;			/* type qualifier	*/
    int		itq;			/* which tq		*/
    int		st	= ty->td.st;	/* symbol type		*/
    int		bt	= ty->td.bt;	/* basic type		*/
    char	*sbVar	= SbInCore (ty->sbVar);  /* var name	*/
    pXTYR	xty = (pXTYR) (ty + 1);	/* XTYR may not be safe	*/

    FLAGT	fC	 = (vlc == lcC);
    FLAGT	fFortran = (vlc == lcFortran);
    FLAGT	fPascal  = (vlc == lcPascal);
    char	sbTemp [200];		/* temporary for holding type	*/ 
    char	sbLeft [200];		/* current "left  side" of type	*/ 
    char	sbRight[200];		/* current "right side" of type	*/
    int		tqPrev = tqNil;		/* previous type qualifier	*/

    long	lbound;			/* array lower bound	*/
    long	hbound;			/* array upper bound	*/
    long	cb;			/* array total size	*/

#ifdef INSTR
    vfStatProc[132]++;
#endif

/*
 * INITIAL WORK WITH VARNAME:
 */

    if ((sbVar == sbNil) OR (*sbVar == chNull))
	 sbVar = vsbunnamed;

    if (fPascal)				/* name goes in front */
    {
	printf ("%s: ", sbVar);
	sbVar = sbNil;				/* don't re-use later */
    }

    sbLeft [0] = chNull;
    sbRight[0] = chNull;

/*
 * PRINT SIMPLE STRUCTURE (no qualifiers):
 */

    if (((ty[0].td.bt   == btStruct)
    OR   (ty[0].td.bt   == btUnion ))
    AND  (TqFTy (ty, 1) == tqNil))
    {
	IssFPxStruct (0L, ty, false);		/* show structure, no data */

	if (! fPascal)
	    printf (" ");			/* will follow with name */
#ifdef S200
	if (fFortran && (*sbVar == '@'))
	    sbVar = sbNil;
#endif
    }

/*
 * CONSTRUCT BASETYPE OR NON-SIMPLE STRUCTURE (e.g. array, or ptr to struct):
 */

    else
    {
	if (ty->td.fConstant)
	    printf ((nl_msg(98, "(const) ")));		/* before anything else */

/*
 * MODIFY TYPENAMES IF NEEDED SO THEY LOOK RIGHT:
 */

	if (fFortran)
	{
	    switch (bt)		/* default: don't change bt */
	    {
		case btShort:	bt = btInt2;	break;
		case btInt:	/* fall through */
		case btLong:	bt = btInt4;	break;
		case btUChar:	bt = btLog1;	break;
		case btUShort:	bt = btLog2;	break;
		case btUInt:	/* fall through */
		case btULong:	bt = btLog4;	break;
		case btFloat:	bt = btReal4;	break;
		case btDouble:	bt = btReal8;	break;
	    }
	}
	else if (fPascal)
	{
#ifdef XDB
            if (((bt == btUChar) || (bt == btUShort) ||
                 (bt == btUInt)  || (bt == btULong)) &&
                (xty->btX == TX_BOOL)) {
               bt = btBoolean;
            }
            else {
#endif
	       switch (bt)		/* default: don't change bt */
	       {
		   case btInt:	/* fall through */
		   case btLong:		bt = btInteger;		break;
		   case btUInt:	/* fall through */
		   case btULong:	bt = btUInteger;	break;
		   case btFloat:	bt = btReal;		break;
		   case btDouble:	bt = btLongReal;	break;
		   case btStruct:	bt = btRecord;		break;
		/* do not map btUnion; it's different */
	       }
#ifdef XDB
	    }
#endif
	}

	if (bt == btNil)			/* might be extended type */
	{
	    switch (xty->btX)			/* if so, convert bt */
	    {
#ifdef HPSYMTABII
		case T_FTN_STRING_SPEC:         /* fall through */
		case T_MOD_STRING_SPEC:
#endif
		case T_STRING200:		/* fall through */
#ifdef HPSYMTAB
		case T_LONGSTRING200:		/* fall through */
#endif
		case T_STRING500:
		                   bt = fFortran ? btStrHPF : btStrHPP;
							break;
		case T_TEXT:	   bt = btText;		break;
		case T_FLABEL:	   bt = btFLabel;	break;
		case TX_SET:	   bt = btSet;		break;
#if (XDB && HPSYMTABII)
		case T_PACKED_DECIMAL:	bt = btPackedDec;	break;
#endif
	    }
	}

	if (fPascal)
	    sprintf (sbRight, "%s",  vmpBtSb [bt]);	/* save it */
	else
	    printf  ("%s ", vmpBtSb [bt]);		/* dump it */


/* for HPSYMTABs the tagname is not easily accessible, so not printed */

    } /* else */

/*
 * CONSTRUCT PREFIX AND SUFFIX (QUALIFIER) INFORMATION FOR PASCAL:
 *
 * Go highest to lowest and print directly (bt is already saved on the
 * right side).  Var name was already printed.
 */

    if (fPascal)
    {
	for (itq = 1; itq <= 6; itq++)		/* highest to lowest */
	{
	    if ((tq = TqFTy (ty, itq)) == tqNil)	/* no more */
		break;

	    switch (tq)				/* no need for default */
	    {
		case tqPtr:	printf ("^");			break;
		case tqFunc:	printf ((nl_msg(99, "function returning ")));	break;
		/* a misnomer (they're all procs), but this is C-compatible */

		case tqArray:
		    printf ("array ");
		    DimFTy (ty, itq, & cb, & lbound, & hbound);

		    if (hbound == intMax)		/* unknown */
			printf ("[]");
		    else
			printf ("[%d..%d]", lbound, hbound);

		    printf (" of ");
		    break;

	    } /* switch */
	} /* for */
    } /* if */

/*
 * NON-PASCAL:
 *
 * Build up type notation left and right of the variable name, going
 * inwards, lowest to highest priority.  Bt is already on the left.
 */

    else
    {
	for (itq = 6; itq >= 1; itq--)		/* lowest to highest */
	{
	    if ((tq = TqFTy (ty, itq)) == tqNil)	/* none yet */
		continue;

	    switch (tq)				/* no need for default */
	    {
		case tqPtr:			/* DATA POINTER */
		    if (tqPrev == tqArray)
		    {
			strcat (sbLeft, "(*");	/* append to left   */
			strcpy (sbTemp, ")");	/* prepend to right */
			strcat (sbTemp, sbRight);
			strcpy (sbRight, sbTemp);
		    }
		    else
			strcat (sbLeft, "*");	/* just append to left */

		    tqPrev = tqPtr;
		    break;

		case tqFunc:			/* FUNCTION POINTER */
#ifdef HPSYMTABII
		    strcat (sbLeft, "(");	/* append to left   */
#else
		    strcat (sbLeft, "(*");	/* append to left   */
#endif
		    strcpy (sbTemp, ")()");	/* prepend to right */
		    strcat (sbTemp, sbRight);
		    strcpy (sbRight, sbTemp);
		    tqPrev = tqPtr;		/* treat like *data */
		    break;

		case tqArray:			/* ARRAY */
		    /* put array bounds description in sbTemp */

		    DimFTy (ty, (fFortran ? -itq : itq),
			    & cb, & lbound, & hbound);

		    if (hbound == intMax)	/* unknown */
			sprintf (sbTemp, fFortran ? "()" : "[]");

		    else if (fC)
		    {
			if (lbound == 0)		/* C style */
			    sprintf (sbTemp, "[%d]",    hbound + 1);
			else				/* hybrid style */
			    sprintf (sbTemp, "[%d:%d]", lbound, hbound);
		    }
		    else				/* FORTRAN style */
			sprintf (sbTemp, "(%d:%d)",  lbound, hbound);

		    /* Pascal done elsewhere */

		    /* now prepend array description to right side */
		    strcat (sbTemp, sbRight);
		    strcpy (sbRight, sbTemp);
		    tqPrev = tqArray;
		    break;

	    } /* switch */
	} /* for */
    } /* else */

/*
 * PRINT DESCRIPTION (left part, name, right part):
 *
 * Any part might be a nil string.
 */

    printf ("%s%s%s", sbLeft, sbVar, sbRight);

    if (ty->td.width != 0)			/* a bit field */
	printf (" : %d", ty->td.width);		/* tell size   */

} /* PxTy */


/***********************************************************************
 * I S S   F   P X   S T R   F I E L D S
 *
 * Print a "picture" of a structure's fields, optionally with
 * values.  Called by IssFPxStruct and IssFPxVariant.
 */

void IssFPxStrFields (adr, tyStruct, fDoVal, fStruct)
    ADRT	adr;		/* adr of structure */
    pTYR	tyStruct;	/* type information */
    FLAGT	fDoVal;		/* show values?	    */
    FLAGT	fStruct;	/* struct? (false => union */
{
    char	*sbIndent = "    ";	/* indent format */
    pXTYR	xty = (pXTYR) (tyStruct + 1);	/* safe -- not btDouble	*/
    DNTTPOINTER nextfield;			/* next to show		*/
    DNTTPOINTER vartagfield;			/* variant tag field	*/
    DNTTPOINTER varlist;			/* list of variants	*/
    long	adrLong;			/* address of field	*/
    TYR		tyField[cTyMax];		/* to build field info	*/
    int		cNest;				/* nesting depth	*/
    MODER	modeDummy;			/* has same effect as   */
						/* modeNil when passed  */
						/* to DispVal		*/

#ifdef INSTR
    vfStatProc[133]++;
#endif

    if (fStruct)
    {
	nextfield	= vsymCur->dstruct.firstfield;
	vartagfield	= vsymCur->dstruct.vartagfield;
	varlist		= vsymCur->dstruct.varlist;
    }
    else
    {
	nextfield		= vsymCur->dunion.firstfield;
	vartagfield.word	= DNTTNIL;
	varlist.word		= DNTTNIL;
    }

/*
 * DO EACH FIELD OF STRUCTURE:
 */

    while (nextfield.word != DNTTNIL)		/* not done yet */
    {
	SetSym (nextfield.dnttp.index);		/* start next field   */
	nextfield = vsymCur->dfield.nextfield;	/* save now for later */
#ifdef S200
	if (!vsymCur->dfield.bitlength) continue;/* ignore FORTRAN PARAMETERs */
#endif

						/* set up field information */
	tyField[0].sbVar = SbSafe (SbInCore (vsymCur->dfield.name));
	tyField[0].td.st = stStruct;

	adrLong = AdrFField (adr, tyStruct, tyField);

	xty = (pXTYR) (tyField + 1);		/* safe -- not btDouble	*/
	xty->isymRef = visym;			/* note current symbol	*/

	for (cNest = 0; cNest < vcNest; cNest++)   /* print indentation */
	    printf (sbIndent);

	if (fDoVal)
	{
	  /*
	   * do values -- pass modeDummy instead of modeNil so that vmode
	   * is not changed.  vmode should reflect outermost structure when
	   * we're done.
	   */
	    modeDummy.df = dfNil;		/* These values are the	*/
	    modeDummy.len = 0;			/* the same as in	*/
	    modeDummy.cnt = 1;			/* modeNil.		*/
	    modeDummy.imap = 0;
	    DispVal (adrLong, tyField, &modeDummy, false, true, true);
	}
	else					/* print type only */
	    PxTy (tyField);

#ifdef S200
	if (vlc == lcFortran) 
	    printf ("\n");
	else
#endif
	printf (";\n");				/* correct for both cases */

    } /* while */

/*
 * DO ANY VARIANTS
 */

    if (varlist.word != DNTTNIL)
	IssFPxVariant (adr, tyStruct, fDoVal, vartagfield, varlist);

} /* IssFPxFields */


/***********************************************************************
 * I S S   F   P X   V A R I A N T
 *
 * Print a "picture" of a structure's variant, optionally with 
 * values.  Called by IssFPxStrFields.
 */

void IssFPxVariant (adr, tyStruct, fDoVal, vartagfield, varlist)
    ADRT	adr;		/* adr of structure */
    pTYR	tyStruct;	/* type information */
    FLAGT	fDoVal;		/* show values?	    */
    DNTTPOINTER vartagfield;			/* variant tag field	*/
    DNTTPOINTER varlist;			/* list of variants	*/
{
    char	*sbIndent = "    ";	/* indent format */
    pXTYR	xty = (pXTYR) (tyStruct + 1);	/* safe -- not btDouble	*/
    DNTTPOINTER varstruct;			/* variant structure	*/
    DNTTPOINTER vartype;			/* variant's type	*/
    long	adrLong;			/* address of field	*/
    TYR		tyField[cTyMax];		/* to build field info	*/
    int		cNest;				/* nesting depth	*/
    MODER	modeDummy;			/* has same effect as   */
						/* modeNil when passed  */
						/* to DispVal		*/
    FLAGT	fTagPrinted;			/* printed tag yet?	*/
    long	lowVarValue;			/* variant tag value	*/
    long	hiVarValue;			/* variant tag value	*/
    long	bt;				/* type			*/
						/* to DispVal		*/
    STUFFU      stuff;				/* to fetch value of tag*/
    int         tagval;				/* value of tag         */
    FLAGT       fhavetagval = false;
    FLAGT       fhavetag1 = false;
    static long tag1typeisym = isymNil;
    static long tag1val;

#ifdef INSTR
    vfStatProc[134]++;
#endif

    fTagPrinted = false;
    for (cNest = 0; cNest < vcNest; cNest++)	/* print indentation	*/
	printf (sbIndent);
    printf ("case ");

/*
 * PRINT TAG FIELD, IF ANY
 */
    if ((vartagfield.word != DNTTNIL) && !vartagfield.dntti.immediate)
    {
	SetSym (vartagfield.dnttp.index);

     if (vsymCur->dblock.kind != K_FIELD)  
     {                   /* no TAG FIELD NAME, actually TYPE of TAG FIELD */
        vartype = vartagfield;
        if (tag1typeisym == visym) {
            tagval = tag1val;
            fhavetagval = true;
        }
     }
     else
     {
        vartype = vsymCur->dfield.type;
	/* set up field information */
	tyField[0].sbVar = SbSafe (SbInCore (vsymCur->dfield.name));
	tyField[0].td.st = stStruct;

	/* Get address and type of field -- will change vsymCur */
	adrLong = AdrFField (adr, tyStruct, tyField);

	xty = (pXTYR) (tyField + 1);		/* safe -- not btDouble	*/
	xty->isymRef = visym;			/* note current symbol	*/

	if (fDoVal)
	{
	  /*
	   * do values -- pass modeDummy instead of modeNil so that vmode
	   * is not changed.  vmode should reflect outermost structure when
	   * we're done.
	   */
	    modeDummy.df = dfNil;		/* These values are the	*/
	    modeDummy.len = 0;			/* the same as in	*/
	    modeDummy.cnt = 1;			/* modeNil.		*/
	    modeDummy.imap = 0;
	    DispVal (adrLong, tyField, &modeDummy, false, true, true);
	    ValFAdrTy (&stuff, adrLong, tyField, CbFTy(tyField, true), false);
            tagval = stuff.lng;
            fhavetagval = true;
            if (tag1typeisym == isymNil) {
              fhavetag1 = true;
	      tag1typeisym = vartype.dnttp.index;
              tag1val = tagval;
	   }
	}
	else					/* print type only */
	    PxTy (tyField);

	printf (" of\n");
	fTagPrinted = true;
     }
    }

/*
 * SKIP TO VARIANT SPECIFIED BY TAG VALUE (IF ANY)
 */
    if (fhavetagval) {
        while (varlist.word != DNTTNIL) {
            SetSym (varlist.dnttp.index);
            lowVarValue = vsymCur->dvariant.lowvarvalue;
            hiVarValue = vsymCur->dvariant.hivarvalue;
            if ((tagval >=  lowVarValue) &&
                (tagval <= hiVarValue)) {
                break;
            }
	    varlist = vsymCur->dvariant.nextvar;
            if (varlist.word == DNTTNIL) {
	        for (cNest = 0; cNest < vcNest; cNest++) printf (sbIndent);
                printf((nl_msg(90,"No variant for tag value\n")));
            }
        }
    }
/*
 * PRINT EACH VARIANT
 */

    while (varlist.word != DNTTNIL)
    {
        SetSym (varlist.dnttp.index);
	varlist = vsymCur->dvariant.nextvar;
        if (fhavetagval) {
           varlist.word = DNTTNIL;
        }
	varstruct = vsymCur->dvariant.varstruct;
	lowVarValue = vsymCur->dvariant.lowvarvalue;
	hiVarValue = vsymCur->dvariant.hivarvalue;

    /*
     * PRINT TAG TYPE IF NO TAG FIELD
     */
	if (! fTagPrinted)
	{
	    /* map HP type to bt field -- may move vsymCur */
            if (vartagfield.dntti.immediate) {
               vartype = vartagfield;
            }
	    TyFScanHp (tyField, vartype);
	    bt = tyField->td.bt;
	    switch (bt)		/* default: don't change bt */
	    {
		case btInt:	/* fall through */
		case btLong:	bt = btInteger;		break;
		case btUInt:	/* fall through */
		case btULong:	bt = btUInteger;	break;
	    }
	    printf ("%s of\n",  vmpBtSb [bt]);	/* print type */
	    fTagPrinted = true;
	}

    /*
     * PRINT VARIANT TAG VALUE
     */

	for (cNest = 0; cNest < vcNest; cNest++)   /* print indentation */
	    printf (sbIndent);

	if (tyField->td.bt == btChar)
	{
	    if (lowVarValue == hiVarValue)
	        printf ("\'%c\'", lowVarValue);
	    else
		printf ("\'%c\'..\'%c\'", lowVarValue, hiVarValue);
	}
	else if (tyField->td.bt == btEType)
	{
	    PxEnum (lowVarValue, tyField);
	    if (lowVarValue != hiVarValue)
	    {
		printf ("..");
		PxEnum (hiVarValue, tyField);
	    }
	}
	else
	{
	    if (lowVarValue == hiVarValue)
	        printf ("%d", lowVarValue);
	    else
		printf ("%d..%d", lowVarValue, hiVarValue);
	}
	printf (": (\n");

    /*
     * PRINT FIELDS OF THE VARIANT
     */

	vcNest++;

	if (varstruct.word != DNTTNIL)
	{
	    SetSym (varstruct.dnttp.index);
	    IssFPxStrFields (adr, tyStruct, fDoVal, true);
	}

	vcNest--;
	for (cNest = 0; cNest < vcNest; cNest++)   /* print indentation */
	    printf (sbIndent);
	printf (")\n");
	
    }  /* while */

} /* IssFPxVariant */


/***********************************************************************
 * P X   S E T
 *
 * Print the value of a Pascal set
 */

void PxSet (ty, adr)
    pTYR	ty;
    ADRT	adr;		/* address of data		*/
{
    register long curvalue;	/* "current" item value		*/
    register long hivalue;	/* last item value of base type */
    long	basetype;	/* basetype of set		*/
    register long bitinx;	/* bit index in word		*/
    register long mask;		/* mask to test bits in word	*/
    register long data;		/* data buffer			*/
    long	buf;		/* data buffer			*/
    pXTYR	xty = (pXTYR) (ty + 1);
    FLAGT	fPrint;		/* any members printed yet?	*/
    long	bitoffset;	/* bit offset of set in word 	*/
#ifdef HPSYMTABII
    int         decl;           /* is set  packed or crunched?  */
    int         unusedbits;     /* unused bits in front of set  */
#endif

#ifdef INSTR
    vfStatProc[135]++;
#endif

    SetSym (xty->isymRef);	/* cur sym == DNTT entry for set */
#ifdef HPSYMTABII
    decl = vsymCur->dset.declaration;
#endif
    if (vsymCur->dset.subtype.dnttp.immediate)
    {
	if (vsymCur->dset.subtype.dntti.type == T_CHAR)
	{
	    basetype = T_CHAR;
	    curvalue = 0;
	    hivalue = 255;
	}
	else
	{
	    Panic ((nl_msg(521, "Internal Error (IE521)")));
	}
    }
    else
    {
	SetSym (vsymCur->dset.subtype.dnttp.index);
	while (vsymCur->dblock.kind == K_TYPEDEF)  /* skip thru TYPE records */
	    SetSym (vsymCur->dtype.type.dnttp.index);

	if (vsymCur->dblock.kind == K_SUBRANGE)	 /* base type == subrange */
	{
	    basetype = K_SUBRANGE;
#ifdef HPSYMTABII
	    curvalue = vsymCur->dsubr.lowbound;
	    hivalue = vsymCur->dsubr.highbound;
            if (decl == DECLCRUNCHED) {
               unusedbits = 0;
            }
            else {
               if (decl == DECLPACKED) {
                  if ((hivalue - curvalue) < SZCHAR) {
                     unusedbits = curvalue & (SZCHAR - 1);
                  }
                  else {
                     if ((hivalue - curvalue) < SZSHORT) {
                        unusedbits = curvalue & (SZSHORT - 1);
                     }
                     else {
                        unusedbits = curvalue & (SZLONG - 1);
                     }
                  }
               }
               else {
                  unusedbits = curvalue & (SZLONG - 1);
               }
            }
            adr += (ty->valTy + unusedbits) / SZCHAR;
            ty->valTy = (ty->valTy + unusedbits) & (SZCHAR - 1);
            
            if (!vsymCur->dsubr.subtype.dnttp.immediate) { /* must be enum */
               int imem; /* used to adjust to first mem within subrange */
               basetype = K_ENUM;
               SetSym(vsymCur->dsubr.subtype.dnttp.index);
	       SetSym (vsymCur->denum.firstmem.dnttp.index);
               for (imem = 0; imem < curvalue; imem++) {
		  SetSym (vsymCur->dmember.nextmem.dnttp.index);
               }
            }
#else /* not HPSYMTABII */
#ifdef S200
	    curvalue = 0;
#else
	    curvalue = vsymCur->dsubr.lowbound;
#endif
	    hivalue = vsymCur->dsubr.highbound;
#endif /* not HPSYMTABII */
	}
	else if (vsymCur->dblock.kind == K_ENUM)	/* base type == enum */
	{
	    basetype = K_ENUM;
	    SetSym (vsymCur->denum.firstmem.dnttp.index);
	    curvalue = vsymCur->dmember.value;
	    hivalue = intMax;
	}
	else					/* no other valid basetypes */
	{
	    Panic ((nl_msg(521, "Internal Error (IE521)")));
	}
    }

#ifdef S200
    buf = 0;
    GetBlock (adr, spaceData, ((char *)(&buf)) + 2, 2);
    if (hivalue > 255)
       hivalue = curvalue + buf * 8 - 1;	/* only look at data bytes */
    adr += 2;					/* skip 2 bytes of length */
#endif

    printf ("[");
    fPrint = false;
    bitoffset = ty->valTy;	/* bit offset if set packed into record */
    while (curvalue <= hivalue)
    {
	GetBlock (adr, spaceData, &buf, 4);	/* get one word		*/
	data = buf;				/* move into register 	*/
	for (bitinx = 31 - bitoffset;
	     (bitinx >= 0) && (curvalue <= hivalue);
	     bitinx--)
	{
	    mask = 1 << bitinx;
	    if (data & mask)			/* element in set	*/
	    {
		if (fPrint)
		    printf (", ");
		if (basetype == T_CHAR)
		{
		    if ((curvalue < ' ') OR (curvalue > 0176))	/* octal */
			printf ("'\\%#lo'", curvalue);
		    else				/* just quoted char */
			printf ("'%c'", curvalue);
		}
		else if (basetype == K_SUBRANGE)
		{
		    printf ("%d", curvalue);
		}
		else  /* enum */
		{
		    printf ("%s", NameFCurSym ());
		}
		fPrint = true;
	    }
	    if ((basetype == T_CHAR) OR (basetype == K_SUBRANGE))
	    {
		curvalue++;
	    }
	    else
	    {
		if (vsymCur->dmember.nextmem.word == DNTTNIL)
		{
		    hivalue = curvalue - 1;	/* to get us out of loop */
		}
		else
		{
		    SetSym (vsymCur->dmember.nextmem.dnttp.index);
		    curvalue = vsymCur->dmember.value;
		}
	    }
	} /* for */
	adr += 4;
	bitoffset = 0;
    } /* while */
    printf ("]");
	
} /* PxSet */

#ifdef FOCUS
/**************************************************************************
 * P R I N T   I N S T
 *
 * Print disassembled instruction -- stub routine on Series 500
 */

long PrintInst()
{
}
#endif /* FOCUS */
