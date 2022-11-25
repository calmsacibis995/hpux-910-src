/* @(#) $Revision: 70.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines handle the analysis of format specifiers and the printing
 * of data values according to formats, including structure, enum, and type
 * dumps.
 */

#include <ctype.h>
#include "macdefs.h"
#include "cdb.h"

export	int	vcNest;			/* nest level for printing structs */
local	char	*sbErrFormat = "Invalid display format \"%s\"";
local long IssFPxStruct();

#ifdef JUNK
/* NAMES OF BASIC TYPES (a mapping): */
char *vmpBtSb[] = {
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
};
#endif /* JUNK */

/***********************************************************************
 * D F	 F   T Y
 *
 * Given a type, determine what display format to use for it.
 * This routine implements "alternate forms" for strings and structures.
 * Also, it always returns a valid df, never dfNil.
 */

local DFE DfFTy (ty, fAlternate)
    pTYR	ty;			/* type to evaluate	  */
    FLAGT	fAlternate;		/* treat strings as such? */
{
    int		bt = ty->td.bt;		/* base type		*/
    int		tq = TqFTy (ty, 1);	/* curr type qualifier	*/

/*
 * CHECK FOR ALTERNATE FORM:
 */

    if (fAlternate					/* alternate form OK */
    AND (tq == tqArray)					/* pointer or array  */
    AND (TqFTy (ty, 2) == tqNil)			/* just * or []	     */
    AND (bt == btChar))					/* char* or char[]   */
    {
	return (dfStr);					/* use alternate */
    }

/*
 * CHECK FOR ARRAY, POINTER, OR FUNC:
 */

    if (tq != tqNil)					/* has a prefix  */
	return (dfAddr);				/* will show adr */


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
	case btULong:	return (dfHex);

	default:				/* note well */
	case btNil:
	case btShort:
	case btInt:
	case btFArg:
	case btEMember:
	case btLong:	
			return (dfHex);

	case btFloat:
	case btDouble:	
#ifdef KFLPT
			return (dfGFloat);
#else
			return (dfHex);
#endif

	case btEType:	return (dfEnum);

	case btChar:	return (dfChar);

    } /* switch */

} /* DfFTy */


/***********************************************************************
 * D F	 F   C H
 *
 * Map a format character to the corresponding display format.  "n"
 * (normal) format maps into dfNil, the same as if no format was given.
 * In case of error, assumes vsbTok has the string ch came from.
 */

local DFE DfFCh (ch)
    char	ch;	/* format char to use */
{
    switch (ch)
    {
	default:	UError (sbErrFormat, vsbTok);

	case 'a':	return (dfStr);

	case 'c':	return (dfChar);
	case 'C':	return (dfCChar);

	case 'b':
	case 'd':	return (dfDecimal);

#ifdef KFLPT
	case 'e':
	case 'E':	return (dfEFloat);

	case 'f':
	case 'F':	return (dfFFloat);

	case 'g':
	case 'G':	return (dfGFloat);
#endif

	case 'i':	return (dfInst);

	case 'n':	return (dfNil);		/* note -- as if no format! */

	case 'o':	return (dfOctal);

	case 'p':	return (dfProc);

	case 's':	return (dfPStr);

	case 'S':	return (dfStruct);

	case 'u':	return (dfUnsigned);

	case 'x':	return (dfHex);

    } /* switch */

} /* DfFCh */


/***********************************************************************
 * C B	 F   C H
 *
 * Map a format character to the size of the format (in bytes).  In case
 * of error, assumes vsbTok has the string ch came from.  Returns -1 for
 * "use a size based on the size of the underlying type, but not larger
 * than CBLONG" (via CbFTy()).  Returns -2 for "use a size based on the
 * size of the underlying type, and the user cannot give a size".
 */

local int CbFCh (ch)
    char	ch;		/* format character to use */
{
    switch (ch)
    {
	default:	UError (sbErrFormat, vsbTok);

	case 'b':
	case 'c':
	case 'C':	return (CBCHAR);

#ifdef KFLPT
	case 'e':
	case 'f':
	case 'g':	return (CBFLOAT);

	case 'E':
	case 'F':
	case 'G':	return (CBDOUBLE);
#endif

	case 'i':	return (0);

	case 'd':
	case 'o':
	case 'u':
	case 'x':
	case 'a':
	case 's':	return (-1);		/* use CbFTy() size */

	case 'n':
	case 'p':
	case 't':
	case 'S':	return (-2);		/* user size not allowed */

    } /* switch */

} /* CbFCh */


/***********************************************************************
 * G E T   M O D E
 *
 * Analyze the next few tokens as variable formatting info.  We expect
 * the current token (which is skipped) to be "/", "?", or "^", followed
 * by "[*] [[count] formchar [size]]", where count is numeric, formchar
 * is alpha, and size is numeric or a special letter.  Note that formchar
 * and size are tokenized as one token, e.g. "x12" or "xb".  Leaves the
 * current token set to the last one used, in keeping with commands.
 *
 * If no size is given, the default size from CbFCh() is used, which
 * might be -1 (base size on type, not larger than CBLONG) or -2 (same,
 * but user can't give a size).  Otherwise the size can be any positive
 * number; good luck to the user if it's not 1, 2, or 4.
 *
 * Because of these special return values, IncFTyMode() should not be
 * called until after DispVal() massages the mode record.
 */

export void GetMode (mode)
    register pMODER	mode;	/* where to return info */
{
    register char	ch;	/* misc usage */
    long	cnt;		/* temp value */
    long	len;		/* temp value */
    int		ich;		/* char index */
    FLAGT	fFormatReq = ((vtk == tkSlash) OR (vtk == tkQuest));

    mode->imap = 0;

    TkNext();					/* set past "/|?|^" */

    if (FAtSep (vtk))				/* end of command */
    {
	if (fFormatReq)				/* format must be present */
	    UError ("Format is missing");
	else
	    return;				/* no error for "^" case */
    }

    if (vtk == tkStar)				/* USE ALTERNATE MAP */
    {
	mode->imap = 1;
	TkNext();				/* set to next (unknown) */

	if (FAtSep (vtk))			/* no more info */
	    return;
    }

    if (FValFTok (& cnt))			/* COUNT IS GIVEN */
    {
	mode->cnt = cnt;			/* might shorten it */
	TkNext();				/* set to unknown */
    }
    else
	mode->cnt = 1;				/* use default */

    if (vtk != tkStr){				/* INVALID FORMAT */
	UError (sbErrFormat, vsbTok);
    }

    /* here is an experiment in allowing types as formats */
    visym = isymNil;
    while (FNextSym(K_TYPEDEF, K_TAGDEF, K_NIL, K_NIL, vsbTok, FSbCmp, 
		K_NIL, K_NIL, K_NIL))
    {
	TyFGlobal(vtyDot);
	if (CbFTy(vtyDot) > 0) break;
    }
    if (visym != isymNil)
    {
	mode->len = 0;
	mode->df = dfNil;
	return;
    }
    /* end of experiment */

    ch = vsbTok[0];				/* ANALYZE FORMAT */

    mode->df = DfFCh (ch);			/* get display format */
    len	     = CbFCh (ch);			/* get default size   */

    if (vcbTok > 1)				/* LENGTH IS GIVEN TOO */
    {
	if (len == -2)
	    UError ("Length not allowed with \"%c\" format", ch);

	ch = vsbTok[1];

	if (isdigit (ch))			/* LOOKS LIKE A NUMBER */
	{
	    len = atoi (& vsbTok[1]);

	    for (ich = 2; ich < vcbTok; ich++)	/* insure pure number > 0 */
		if ((len < 1) OR (! isdigit (vsbTok [ich])))
		    UError (sbErrFormat, vsbTok);
	}
	else					/* MUST BE A SPECIAL CHAR */
	{
	    UError (sbErrFormat, vsbTok);
	}
    }
    mode->len = len;

} /* GetMode */


/***********************************************************************
 * I N C   F   T Y   M O D E
 *
 * Given a type and display mode, return the increment (bytes) for
 * successive data displays.  Some types imply an increment, but in the
 * default case the mode->len saved from the last display is used to
 * figure it.  If the saved value is < 0 it was a total size.
 *
 * Because of the special return values from GetMode(), this routine
 * should not be called until after DispVal() massages the mode record.
 */

export int IncFTyMode (ty, mode)
    register pTYR	ty;		/* type to check */
    register pMODER	mode;		/* mode info	 */
{
    register int cbRandom;

    if (ty->td.st == stSpc)		/* don't allow inc's of specials */
	return (0);

    ty->sbVar = -1;			/* toss name -- no longer correct */

    if (ty->td.st == stReg)			/* next register */
	return (1);

    if (mode->df == dfInst)
    {	cbRandom = mode->len;
	mode->len = 0;
	return cbRandom;
    }

    if (mode->len < 0)				/* random size object	*/
	return (-(mode->len));			/* skip what was shown	*/
    else					/* any other type	*/
	return (mode->cnt * mode->len);		/* skip what was shown	*/

} /* IncFTyMode */


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

export void DispVal (adrLong, tyIn, mode, fPadName, fIndirect, fAlternate)
    long	adrLong;		/* address of data to show	*/
    register pTYR	tyIn;		/* data type info, if any	*/
    register pMODER	mode;		/* print mode info, if any	*/
    FLAGT	fPadName;		/* pad value name with blanks?	*/
    FLAGT	fIndirect;		/* indirect via adr, or direct? */
    FLAGT	fAlternate;		/* alternate form OK?		*/
{
    int		count;			/* how many objects to show	*/
    register DFE	df;		/* display format to use	*/
    register int	len;		/* size of one object (bytes)	*/
    int		bitsize;		/* size of one object (bits)	*/
    int		cbTy;			/* results of CbFTy()		*/
    int		btNew	   = btNil;	/* to change bt for "." (dot)	*/
    FLAGT	fRandomLen = false;	/* random size object?		*/
    FLAGT	fSingle;		/* printing just one object?	*/

    int		width;			/* of field for some printf()s	*/
    int		precision;		/* of field for some printf()s	*/
    int		modval = 1;		/* objects per line		*/
    int		cPrint = 0;		/* objects printed so far	*/

    char	*sbVar;			/* name of object		*/
    STUFFU	stuff;			/* value to show, flexible form	*/
    long	val;			/* value or address to show	*/
    long	valMask;		/* value after masking		*/

    ADRT	adrStr;			/* address of string value	*/
    int		cbStr;			/* length of string		*/
    int		lenStr;			/* maximum to display		*/
    char	*sbQuote;		/* how to quote a string	*/

    if ((cbTy = CbFTy (tyIn, false)) < 1)  /* base type size for HPSYMTAB */
	 cbTy = CBINT;	 /* don't care if it's bit field or has zero size */

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

	if ((len >= CBCHAR) AND (len <= CBLONG))
	{
	    switch (len)
	    {
	    case CBCHAR:
			btNew = (df == dfUnsigned) ? btUChar : btChar;
			break;

	    case CBSHORT:
			btNew = (df == dfUnsigned) ? btUShort : btShort;
			break;

	    default:	btNew = (df == dfUnsigned) ? btULong : btLong;

#ifdef KFLPT
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
    /* else if df and len both given, use the given len */


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

#ifdef KFLPT
	case dfEFloat:	/* fall through */
	case dfFFloat:	/* fall through */
	case dfGFloat:	modval =  4;
			width  = 14;
			break;
#endif

	case dfProc:	fIndirect = false;	/* don't look up data */
			/* this also prevents printing of the name!   */
			break;

	case dfStr:	len = CBCHAR;		/* simple string */
			if (fRandomLen)
			    mode->len = 0;	/* init for later decrement */
			break;

	case dfCChar:	modval = 65535;
			cPrint = 1;
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

	vimap = mode->imap;		/* sorta kludgey - sorry */

	if (fIndirect AND (df != dfStruct))	/* get data using address */
	{
	    /* use len, not base size; do not cast to long */
	    ValFAdrTy (& stuff, adrLong, tyIn, len, false, true);
	    val = stuff.lng;			/* but leave stuff as is */

	    if (len == 3)			/* special, see ValFAdrTy() */
		val >>= SZCHAR;
	}
	else					/* just use address itself */
	    val = adrLong;

	vimap = 0;

/*
 * SHOW THE NAME OR ADDRESS if we can and if the time is right:
 */

	if (fIndirect			/* it's meaningful to show */
	AND (cPrint % modval == 0))	/* at start of a new line  */
	{
	    if (fSingle				/* just one object	*/
	    AND (sbVar	 = SbInCore (tyIn[0].sbVar))  /* it has a name	*/
	    AND (*sbVar != chNull)		/* and it's valid	*/
	    AND (*sbVar != '*'))		/* doesn't start as "*"	*/
	    {					/* PRINT THE NAME	*/
		printf (fPadName ? "%-9s = "   : "%s = ",   sbVar);
	    }
	    else if ((TqFTy (tyIn, 1) != tqArray) AND	/* not array */
		     (df != dfInst))			/* not inst  */
	    {
		printf ("%#lx  ", adrLong); /* PRINT ADDRESS */
	    }
	}

/*
 * NOW SHOW THE VALUE:
 */

	valMask	= val & (Mask (bitsize));
	adrStr	= adrLong;		/* default for dfStr */

	switch (df)
	{
	    default:		Panic (vsbErrSinSD, "Unknown display format",
					"DispVal", df);

	    case dfHex:		printf ("%#*.*lx", width, precision, valMask);
				break;
	    case dfOctal:	printf ("%#*.*lo", width, precision, valMask);
				break;
	    case dfAddr:	printf ("%#*lx",   width, val);
				break;
	    case dfUnsigned:	printf ("%*lu",    width, valMask);
				break;
	    case dfDecimal:	printf ("%*ld",    width, val);
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

	  case dfCChar:
		if (valMask > 0176) printf(" ");
		else if (valMask >= ' ') printf("%c",valMask);
		else if (valMask == '\n') printf("\n\r");
		else if (valMask == '\t') printf("\t");
		else printf(" ");
		break;

#ifdef KFLPT
	    case dfEFloat:
	    case dfFFloat:
	    case dfGFloat:
		printf ((df == dfEFloat) ? "%*e" :
			(df == dfFFloat) ? "%*f" : "%*g",
			width, (len <= CBFLOAT) ? stuff.fl : stuff.doub);
		break;
#endif

	    case dfEnum:	PxEnum	     (val, tyIn);		break;
	    case dfStruct:	IssFPxStruct (adrLong, tyIn);		break;
	    case dfProc:	PrintPos     (val, fmtFile + fmtProc + fmtSave);
									break;
	    case dfPStr:
		adrStr = val;		/* do indirection, fall through */

	    case dfStr:			/* fall through */
		if (adrStr == 0)
		{
		    printf ("<null pointer>");
		    break;				/* stop now */
		}

		sbQuote = "\"";
		printf (sbQuote);			/* open quote */

		lenStr = fRandomLen ? 128 : len;	/* maximum or actual */

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

	    case dfInst:
		vimap = mode->imap;
		cbStr = kdbprntins(adrLong);
		printf ("\n");
		vimap = 0;
		fRandomLen = 0;
		mode->len += cbStr;
		break;

	} /* switch */

/*
 * NOTE ACTUAL SIZE for random size object:
 */

	if (fRandomLen)
	{
	    cbStr += 1;			/* move past chNull */

	    if (df == dfStr)		/* remember size for IncFTyMode()  */
		mode->len -= cbStr;	/* HPSYMTAB strings done elsewhere */
	}

/*
 * QUIT LOOP OR ADVANCE TO THE NEXT OBJECT TO SHOW:
 */

	if (--count <= 0) {		/* no more to do */
	    break;
	}

	tyIn->sbVar = -1;		/* invalidate name of named object */

	adrLong += (df == dfStr OR df == dfInst) ? cbStr :
		   (df == dfPStr) ? CBPOINT : len;

/*
 * FINISH LINE OR APPEND BLANK:
 */
	
	if ((df != dfProc) AND (df != dfInst) AND (df != dfCChar))
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


} /* DispVal */


/***********************************************************************
 * P X	 E N U M
 *
 * Print a "picture" of an enumeration member (print its name), given
 * the value and the type info for the enumeration.  Only called by
 * DispVal() to look up the name of a member from its value.  The caller
 * must finish the line (print the newline).
 */

local void PxEnum (val, ty)
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

    SetSym (xty->isymRef);			/* set to K_ENUM */
    if ((vsymCur->dblock.kind == K_TAGDEF) ||
      (vsymCur->dblock.kind == K_TYPEDEF))
	SetSym(vsymCur->dtag.type.dnttp.index);

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
    printf ("%d (value not defined for \"enum %s\")",
	val, SbInCore (ty->sbVar));

    SetSym (isymSave);
    return;


} /* PxEnum */


/***********************************************************************
 * I S S   F   P X   S T R U C T
 *
 * Print a "picture" of a structure and its fields, with
 * values.  Called by DispVal() for simple (non-array,
 * non-pointer) structure/union dumps, and calls one of them in turn
 * for each field (with or without data shown).  (The non-HPSYMTAB
 * version still calls itself directly for simple nested structures.)
 *
 * The caller must provide adr and tyStruct for a btStruct or btUnion,
 * must print "<name> =" first (or "<name>" after) for the tyStruct if
 * the name is desired, and must finish the last line (e.g. ";\n").
 */

local long IssFPxStruct (adr, tyStruct)
    ADRT	adr;		/* adr of structure */
    pTYR	tyStruct;	/* type information */
{
    char	*sbIndent = "    ";	/* indent format */


/*
 * We do the printing, but don't return an iss value.
 * Resets current symbol when done.
 */

    long	isymSave = visym;		/* to reset later	*/
    pXTYR	xty = (pXTYR) (tyStruct + 1);	/* safe -- not btDouble	*/
    FLAGT	fStruct;			/* struct, not union?	*/
    DNTTPOINTER nextfield;			/* next to show		*/
    long	adrLong;			/* address of field	*/
    TYR		tyField[cTyMax];		/* to build field info	*/
    int		cNest;				/* nesting depth	*/
    MODER	modeDummy;			/* has same effect as   */
						/* modeNil when passed  */
						/* to DispVal		*/
    char	*tagname;			/* tag name of struct   */


/*
 * Make sure this is a legitimate request
 */
    if (((tyStruct->td.bt != btStruct) AND
	 (tyStruct->td.bt != btUnion)) OR 
	(tyStruct->td.tq1 != tqNil))
	UError ("This does not appear to be a struct or union");

/*
 * PRINT START OF STRUCTURE:
 */

    tagname = sbNil;
    SetSym (xty->isymRef);				/* to struct or union */

    if ((vsymCur->dblock.kind == K_TAGDEF) ||		/* display type/tag name if we	*/
      (vsymCur->dblock.kind == K_TYPEDEF))		/* can				*/
    {
	tagname = SbInCore(vsymCur->dtag.name);		/* save the name, printed below	*/
	SetSym(vsymCur->dtag.type.dnttp.index);		/* index to struct/union	*/
    }

    fStruct   = (vsymCur->dblock.kind == K_STRUCT);	/* which kind is it?  */

    nextfield = (fStruct ? vsymCur->dstruct.firstfield :
			   vsymCur->dunion .firstfield);

    printf ("%s ", fStruct ? "struct" : "union");

    if (tagname != sbNil)
	printf("%s {\n", tagname);
    else
	printf("{\n");

    vcNest++;			/* up nesting level for indent printing */

/*
 * DO EACH FIELD OF STRUCTURE:
 */

    while (nextfield.word != DNTTNIL)		/* not done yet */
    {
	SetSym (nextfield.dnttp.index);		/* start next field   */
	nextfield = vsymCur->dfield.nextfield;	/* save now for later */

						/* set up field information */
	tyField[0].sbVar = SbSafe (SbInCore (vsymCur->dfield.name));
	xty = (pXTYR) (tyField + 1);		/* safe -- not btDouble	*/
	xty->isymRef = isymNil;
	adrLong = AdrFFieldSym (adr, tyStruct, tyField, visym);
	if (!fStruct)
	    adrLong = adr;

	if (xty->isymRef == isymNil)
	    xty->isymRef = visym;		/* note current symbol	*/

	for (cNest = 0; cNest < vcNest; cNest++)   /* print indentation */
	    printf (sbIndent);

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
	printf (";\n");

    } /* while */

/*
 * PRINT CLOSE OF STRUCTURE:
 */

    vcNest--;

    for (cNest = 0; cNest < vcNest; cNest++)
	printf (sbIndent);

    printf ("}");

    SetSym (isymSave);

    return;


} /* IssFPxStruct */
