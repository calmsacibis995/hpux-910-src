/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/sym.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:23:29 $
 */

/*
 * Original version based on: 
 * Revision 63.3  88/05/26  13:45:08  13:45:08  markm
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
 * This large collection of routines is mostly for raw symbol and string
 * handling (e.g. as read from the object file).  There are also comparison
 * and initialization routines.
 */

#include "cdb.h"

#if (S200BSD && !SYSVHDRS)
#include <nlist.h>
#endif

/*
 * These are used for cacheing SYMBOL INFORMATION.
 *
 * For HPSYMTAB(II), "sym" refers to the DNTT, not the LST, and visymMax
 * is the number of DNTT blocks, not the number of symbols.
 */

#define cbProcMax	20	/* max proc name size		  */
#define iCacheMax	1000	/* number of symbols in cache	  */

long	vcbSymFirst;	/* byte offset to first symbol in symfile */

pSYMR	vsymCur;	/* the "current" symbol */
long	visym;		/* current symbol index */
static	long	visymLo;	/* first sym in cache	*/
static	long	visymLim;	/* last	 sym in cache	*/
long	visymMax;	/* number of symbols	*/

#ifdef HPSYMTABII
long	visymGlobal;	/* isym of first global */
long	vipxdbheader;	/* offset in a.out of pxdb header */
long	vipdfdmd;	/* offset in a.out of quick-lookup tables */
#endif

static	SYMBLOCK vsymCache[iCacheMax];	/* symbol cache */

#define cbSymCache ((long)(sizeof (vsymCache)))

char	*vsbSymfile;	/* name of the symfile	*/
char	*vsbCorefile;	/* name of the corefile */
int	vfnSym;		/* symbol file number	*/
int	vfnCore;	/* corefile number	*/

FLAGT	vfcaseMod;	/* true for case INsensitive search */

#ifdef HPSYMTAB
pSDR	vrgSd;		/* array of scope descriptors	*/
long	visdMac;
static  long	visdMax;
static  long visdIncr;
static  long visdInit;
#endif


/*
 * These are used for caching STRING SPACE info (value table for HPSYMTABs).
 */

int	vcbSbFirst;	/* start of ss in file	*/
int	vcbSbCache;	/* size	 of the cache	*/
static  char *vsbCache;	/* start of the cache	*/
static  char *vsbSs;	/* current place in it	*/
static	int   vissLo;	/* first byte in cache	*/
static	int   vissLim;	/* last	 byte in cache	*/
int	vissMax;	/* last	 byte in file	*/

#ifdef S200
/* S200 LST Proc quick reference array variables */
pLSTPDR	vrgLSTPd;	/* the LST procedure array	*/
long	viLSTPdMax;	/* total number of LST procs	*/
#endif



#ifdef XDB
/************************************************
 *
 *  S Y M   N   L K P
 *
 *  Lookup the symbol name at ofs and re-
 *  turn the name (padded if necessary) in
 *  name.
 */

int symNLkp(adr, name, rmdr)
    int adr;
    char **name;
    unsigned *rmdr;
{

    int i;
    char *s;
    int ipd = IpdFAdr(adr); 		/* only one space for now */

#ifdef INSTR
    vfStatProc[334]++;
#endif

    if (ipd == ipdNil) {
       LabelFAdr(adr,name,rmdr);
       return(1);
    }

    *name = vrgPd[ipd].sbProc;
    *rmdr = adr - vrgPd[ipd].adrStart;

    return(1);
}
#endif


/***********************************************************************
 * F   S B   C M P
 *
 * Compare two strings for equality and return true if they match,
 * including case [in]sensitivity.
 */

FLAGT FSbCmp (sb1, sb2)
    register char    *sb1, *sb2;		/* strings to compare */
{
    register FLAGT fcaseMod = vfcaseMod;	/* in register	    */

#ifdef INSTR
    vfStatProc[335]++;
#endif

    if ((sb1 == sbNil) OR (sb2 == sbNil))	/* one is nil	    */
	return (sb1 == sb2);			/* true if both nil */

    while (true)
    {
        if (fcaseMod)
	{
	    if (tolower(*sb1) != tolower(*sb2))		/* exit if no match */
		break;
	}
	else
	{
	    if (*sb1 != *sb2)				/* exit if no match */
		break;
	}
	if (*sb1 == chNull)				/* end of both */
	    return (true);				/* they match  */
	sb1++;
	sb2++;
    }

    return (false);

} /* FSbCmp */


/***********************************************************************
 * F   H D R   C M P
 *
 * Compare two strings; return true if they match or if the first one
 * is a substring of the other, including case [in]sensitivity.
 */

FLAGT FHdrCmp (sbHdr, sbCheck)
    register char	*sbHdr;		/* pattern to match */
    register char	*sbCheck;	/* string to check  */
{
    register FLAGT	fcaseMod = vfcaseMod;		/* in register	    */

#ifdef INSTR
    vfStatProc[336]++;
#endif

    if ((sbHdr == sbNil) OR (sbCheck == sbNil))		/* one is nil	    */
	return (sbHdr == sbCheck);			/* true if both nil */

    while (true)
    {
        if (fcaseMod)
	{
	    if (tolower(*sbHdr) != tolower(*sbCheck))	/* exit if no match */
		break;
	}
	else
	{
	    if (*sbHdr != *sbCheck)			/* exit if no match */
		break;
	}
	if (*sbHdr == chNull)				/* end of both */
	    return (true);				/* they match  */
	sbHdr++;
	sbCheck++;
    }
    return (*sbHdr == chNull);			/* match if substring */

} /* FHdrCmp */


#ifndef S200
/***********************************************************************
 * F   P R O C	 C M P
 *
 * Compare two strings; return true if they match or if the first one
 * is a substring of the other (but AT LEAST cbProcMax bytes long),
 * including case [in]sensitivity.  E.g., if (strlen (sbProc) <= cbProcMax),
 * an exact match is required, like FSbCmp(), else it is like FHdrCmp().
 * This allows us to have "1234567" match "1234567*", but "1234" does not
 * match "12345".
 */

FLAGT FProcCmp (sbProc, sbCheck)
    register char	*sbProc;	/* pattern to match */
    register char	*sbCheck;	/* string to check  */
{
    char	*sbSave = sbProc;	/* note start of pattern */
    register FLAGT	fcaseMod = vfcaseMod;		/* in register	    */

#ifdef INSTR
    vfStatProc[337]++;
#endif

    if ((sbProc == sbNil) OR (sbCheck == sbNil))	/* one is nil	    */
	return (sbProc == sbCheck);			/* true if both nil */

    while (true)
    {
        if (fcaseMod)
	{
	    if (tolower(*sbProc) != tolower(*sbCheck))	/* exit if no match */
		break;
	}
	else
	{
	    if (*sbProc != *sbCheck)			/* exit if no match */
		break;
	}
	if (*sbProc == chNull)				/* end of both */
	    return (true);				/* they match  */
	sbProc++;
	sbCheck++;
    }

    if (sbProc - sbSave >= cbProcMax)		/* substring matched */
	return (true);

    return (false);

} /* FProcCmp */
#endif /* not S200 */


/***********************************************************************
 * S E T   C A C H E
 *
 * If needed, read a chunk of symbols into the cache so the given symbol
 * is the first one in it, and set global values.
 */

void SetCache (isym)
    register long	isym;		/* symbol to set */
{
    long	offset;		/* where to read */
    int		cRead;		/* bytes read in */

#ifdef INSTR
    vfStatProc[338]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

/*
 * We need to be sure extension blocks (if any) are also in the cache.
 * This might force an unnecessary read if isym is near the end of the
 * DNTT (within the size of the largest symbol type), but so it goes.
 */
    if ((isym >= visymLo) AND
	(isym + (long) (((long)(sizeof (SYMR))) / cbSYMR) <= visymLim))
	return;			/* biggest possible symbol is in cache */

    offset = vcbSymFirst + (isym * cbSYMR);

    if (lseek (vfnSym, offset, 0) < 0L)
    {
	sprintf (vsbMsgBuf, (nl_msg(544, "Internal Error (IE544) (%d)")), offset);
	Panic (vsbMsgBuf);
    }

    if ((cRead = read (vfnSym, vsymCache, cbSymCache)) < 0)
    {
	sprintf (vsbMsgBuf, (nl_msg(545, "Internal Error (IE545) (%d)")), cbSymCache);
	Panic (vsbMsgBuf);
    }

    visymLo  = isym;
    visymLim = Min (visymMax, isym + (cRead / cbSYMR));
    return;

} /* SetCache */


/***********************************************************************
 * S E T   S Y M
 *
 * Make sure the given symbol is in the cache, and set global values.
 *
 * For HPSYMTAB(II) it might call itself recursively in order to advance to
 * the next DNTT block which is not an extension block.
 */

void SetSym (isym)
    register long	isym;		/* symbol to set */
{

#ifdef INSTR
    vfStatProc[339]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((isym < 0) OR (isym >= visymMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(546, "Internal Error (IE546) (%d)")), isym);
#ifdef DEBREC
	return;
#endif
	Panic (vsbMsgBuf);
    }

    SetCache (isym);
    visym   = isym;
    vsymCur = (pSYMR) & (vsymCache[isym-visymLo]); /* need cast for HPSYMTABs */

    if (vsymCur->dblock.extension)		/* oops, at extension block */
	SetSym (isym + 1);			/* recurse to go to next    */
    return;

} /* SetSym */

#ifndef HPSYMTAB
/***********************************************************************
 * S E T   N E X T
 *
 * Set to the symbol before the given one, in preparation for searching.
 *
 * For HPSYMTAB(II) it might call itself recursively in order to find the
 * previous DNTT block which is not an extension block (given that the
 * SetSym() routine advances visym forwards if we do hit one).
 */

void SetNext (isym)
    register long	isym;		/* symbol to set */
{

#ifdef INSTR
    vfStatProc[340]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((isym < 0) OR (isym >= visymMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(547, "Internal Error (IE547) (%d)")), isym);
	Panic (vsbMsgBuf);
    }

    if ((visym = isym - 1) >= isym0)	/* not before start of table */
    {
	SetSym (visym);			/* set to previous symbol */

	if (visym >= isym)		/* tried to set to extension block */
	    SetNext (--isym);		/* recurse to go back one more	   */
    }

} /* SetNext */
#endif /* not HPSYMTAB */


#ifdef HPSYMTABII
/***********************************************************************
 * F   N E X T	 S Y M
 *
 * Search symbols forward for the first one with a matching type and
 * optionally a matching name, or with the stop type.
 */

FLAGT FNextSym (typHit1, typHit2, typHit3, typStop, sbHit, Cmp, typHit4, typHit5, typHit6, typHit7)
    int	     typHit1, typHit2, typHit3;		/* search types	     */
    int	     typStop;				/* stop type	     */
    int	     typHit4, typHit5, typHit6, typHit7;/* for HPSYMTAB(II) only */
    char     *sbHit;				/* name req'd if any */
    FLAGT    (* Cmp)();				/* name compare proc */
{
    register long isym = visym + 1;		/* current symbol */
    register int  i;				/* index of sym	  */
    register int  typ;				/* type of symbol */

#ifdef INSTR
    vfStatProc[341]++;
#endif

/*
 * LOOK FOR NEXT SYMBOL:
 */

    while (isym < visymMax)
    {
	SetCache (isym);			/* only read as needed */

	for (i = (isym - visymLo); (isym < visymLim); i++, isym++)
	{
	    vsymCur = (pSYMR) & (vsymCache[i]); /* need cast for HPSYMTAB(II) */

	    typ = vsymCur->dblock.kind;

	    if (vsymCur->dblock.extension)	/* extension block */
		continue;			/* ignore it	   */

/*
 * CHECK SYMBOL TYPE AND NAME:
 */

	    if ( ((typ == typHit1)
	    OR	  (typ == typHit2)
	    OR	  (typ == typHit3)
	    OR	  (typ == typHit4)
	    OR	  (typ == typHit5)
	    OR	  (typ == typHit6)
	    OR	  (typ == typHit7))
	    AND	 ((sbHit == sbNil) OR FNameCmp (sbHit, Cmp)) )
	    {
		visym = isym;			/* found a match */
		return (true);
	    }
	    else if (typ == typStop)		/* found a stop */
	    {
		return (false);
	    }
	} /* for */
    } /* while */

    return (false);			/* off the end of the symbol table */

} /* FNextSym */
#endif  /* HPSYMTABII */

/***********************************************************************
 * S E T   C U R   L A N G
 *
 * Sets the value of "vlc" (the language of precedence for expression 
 * evaluation) to the language of the current procedure if the user has
 * not selected a language whose precedence he wants all expressions
 * evaluated in. 
 */

void SetCurLang()
{

#ifdef INSTR
    vfStatProc[342]++;
#endif

    if (vlcSet == lcDefault)         /* user has not selected a lang. */
    {
        if (vipd != ipdNil)          /* have a valid current proc.    */
        {
            SetSym (vrgPd[vipd].isym);
            switch (vsymCur->dfunc.language)
            {
                default:		/* fall through */
                case LANG_C        :	vlc = lcC;		break;
                case LANG_HPF77    :	vlc = lcFortran;	break;
                case LANG_HPPASCAL :	/* fall through */
                case LANG_HPMODCAL :	vlc = lcPascal;		break;
#ifdef HPSYMTABII
                case LANG_HPCOBOL  :	vlc = lcCobol;		break;
#endif
            }
        }
    }
}  /* SetCurLang */


#ifdef HPSYMTAB

static  FLAGT vfImportSrch;	/* currently searching IMPORT list? */
static  long visymSrch;		/* current isym of search	*/
static	long visdSrch;		/* current isd of search	*/
static  long visdBndSrch;	/* isd of bounds SD		*/
static  long visymSaveSrch;	/* nested visymSrch		*/
static	long visdSaveSrch;	/* nested visdSrch		*/
static  long visdBndSaveSrch;	/* nested visdBndSrch		*/
static  char vsbImport[128];	/* import search pattern	*/


/***********************************************************************
 * S E T   U P   S R C H
 *
 * Set up global variables in preparation for calling FNextVar.
 * Sets visym to beginning of scope
 */

void SetUpSrch (isd)
    register long isd;			/* scope to be searched */
{

#ifdef INSTR
    vfStatProc[344]++;
#endif

    SetSym (vrgSd[isd].sdIsymStart);
    vfImportSrch = false;
    visymSrch = visym;
    visdSrch = isd;
    visdBndSrch = vrgSd[isd].isdChild;
    if (!visdBndSrch)
	visdBndSrch = isd;
} /* SetUpSrch */

/*************************************************************************
 * F I N D   M O D U L E
 *
 * Find a named module, starting from the specified scope
 */

long FindModule (sbName, isdRef)
    register char *sbName;		/* name of module */
    long isdRef;			/* isd of starting scope */
{
    register long isd;			/* isd of parent scope */
    register long isdCur;		/* current search isd */

#ifdef INSTR
    vfStatProc[345]++;
#endif

    isd = vrgSd[isdRef].isdParent;
    while (isd != isdNil)
    {
	isdCur = vrgSd[isd].isdChild;
	while (isdCur != isdNil)
	{
	    if ((vrgSd[isdCur].sdTy == K_MODULE)
	    AND (!strcmp (vrgSd[isdCur].sdName, sbName)))
	    {
		return (isdCur);
	    }
	    else
	    {
		isdCur = vrgSd[isdCur].isdSibling;
	    }
	}
	isd = vrgSd[isd].isdParent;
    }
    return (isdNil);   /* not found */
}  /* FindModule */
#endif

/***********************************************************************
 * F   N E X T	V A R
 *
 * Find the next variable in the current scope.
 *
 * Must be called with visym+1 (see SetNext()) set somewhere in the scope
 * of interest (global, OR local to some function).  If it is set to a
 * FUNCTION, that FUNCTION is treated as being nested (e.g. below the
 * current scope).
 *
 * Searches forward for the next variable of an allowed storage class
 * (kind).  If sbVar is not nil, the variable name must also match that
 * pattern.  Ignores variables in deeper-nested FUNCTION-END pairs and
 * also those with nil STATTYPE fields.
 *
 * As a special case, if the first kind (kind1) is K_FPARAM, and if the
 * current symbol is K_FUNCTION or K_FPARAM, it tries to follow the param
 * chain instead of going serially.  Note that, for this to work, the
 * current symbol (isym) must not be set to an extension block, e.g. if
 * the caller sets up using SetNext(), the argument to it must be of the
 * form "start of symbol + 1".
 *
 * Quits (but leaves visym advanced by one) if it runs out of symbol
 * table (out of global variables) or hits an END for a FUNCTION while
 * not nested (out of local variables; assumes the END marks the end
 * of current scope).
 */

FLAGT FNextVar (kind1, kind2, kind3, kind4, kind5, sbVar)
    KINDTYPE	kind1, kind2, kind3, kind4, kind5;	/* allowed kinds    */
    char	*sbVar;					/* pattern to match */
{
    long	isymNew;		/* for following param chain	*/
    long	isymNest  = isymNil;	/* deeper scope start, if any	*/
    long	isymBegin;		/* for lookup of scope start	*/
    long	isymEnd;		/* for return to scope end	*/
    static long isymFunc;		/* note function isym		*/
    register KINDTYPE	kind;		/* type of current symbol	*/

#ifdef HPSYMTAB

    long	isymFirst = visymSrch;	/* note our starting point	*/
    register long	isymBound;	/* last DNTT entry to search	*/
    register long	i;		/* index of sym	  		*/

#ifdef INSTR
    vfStatProc[346]++;
#endif

  visym = visymSrch;
  while (true)
  {

/*
 * CHECK FOR PARAM CHAIN:
 */

CheckParam:
    if ((kind1 == K_FPARAM)			/* caller wants params	  */
    AND (visym >= isym0)			/* have a current symbol  */
    AND (! vsymCur->dblock.extension))		/* not at extension block */
    {
	while (true)				/* might follow the chain */
	{
	    kind = vsymCur->dblock.kind;
	    
	    if (((kind == K_FUNCTION) OR (kind == K_ENTRY))	/* at func   */
	    AND (vsymCur->dfunc.firstparam.word != DNTTNIL))	/* has chain */
	    {
		isymFunc = visym + 1; /* to avoid recursion with ENTRY */
		isymNew = vsymCur->dfunc.firstparam.dnttp.index;
	    }
	    else if (kind == K_FPARAM)				/* at param   */
	    {
	        if (vsymCur->dfparam.nextparam.word != DNTTNIL) /* more chain */
	        {
		    isymNew = vsymCur->dfparam.nextparam.dnttp.index;
	        }
		else					/* no more chain */
		{
		    SetSym (isymFunc);
		    break;
		}
	    }
	    else					/* wrong conditions */
	    {
		SetSym (isymFirst);			/* forget whole thing */
		break;
	    }
	    SetSym (isymNew);				/* assume it's param */

	    if ((sbVar == sbNil) OR FSbCmp (sbVar, NameFCurSym()))
	    {
		visymSrch = visym;
		return (true);				/* we have a match */
	    }

	} /* while */
    } /* if */

    if (kind1 == K_FPARAM) {
	kind1 =  K_NIL;				/* ignore params below */
        if (kind2 == K_NIL) {
            return (false);                     /* nothing else to look for */
        }
    }

/*
 * LOOK FORWARD FOR SYMBOL:
 */

    while (visym < vrgSd[visdSrch].sdIsymEnd)
    {
	isymBound = (visdSrch == visdBndSrch) ? vrgSd[visdSrch].sdIsymEnd :
					vrgSd[visdBndSrch].sdIsymStart - 1;
	while (visym <= isymBound)
	{
	    SetCache (visym);			/* only read as needed */

	    for (i = (visym - visymLo); (visym < visymLim); i++, visym++)
	    {
	        vsymCur = (pSYMR) & (vsymCache[i]); /* need cast for HPSYMTAB */

	        if (! vsymCur->dblock.extension)    /* ignore extension block */
		    break;
	    }
	    if (visym > isymBound)
		break;			/* break if forced into next scope */

    /*
     * CHECK SYMBOL TYPE AND NAME:
     */
	    kind = vsymCur->dblock.kind;

	    if ( ((kind == kind1)
	    OR    (kind == kind2)
	    OR    (kind == kind3)
	    OR    (kind == kind4)
	    OR    (kind == kind5))
	    AND  ((! vfImportSrch) OR vsymCur->ddvar.public)
	    AND  ((kind != K_SVAR) OR (vsymCur->dsvar.location != STATNIL))
	    					/* no adr => does not exist */
	    AND  ((kind != K_CONST) OR (vsymCur->dconst.locdesc != LOC_PTR)
		  OR (vsymCur->dconst.location != STATNIL)) )
	    					/* no adr => does not exist */
	    {
		if ((vfImportSrch)
		AND (vsbImport[0] != '\0')
		AND (FSbCmp (vsbImport, NameFCurSym())))
		{				/* found named import item */
		    visymSrch = visymSaveSrch;
		    visdSrch = visdSaveSrch;
		    visdBndSrch = visdBndSaveSrch;
		    vfImportSrch = false;
		    return (true);
		}
		else if ((sbVar == sbNil) OR FSbCmp (sbVar, NameFCurSym()))
		{
		    visymSrch = visym + 1;
		    return (true);
		}
	    }

	/*
	 * Search imported module if names match and we can find the
	 * module.
	 */
	    if ((kind == K_IMPORT)
	    AND (! vfImportSrch))
	    {
		strncpy (vsbImport, SbInCore (vsymCur->dimport.item),
		    sizeof(vsbImport) - 1);
		vsbImport[sizeof(vsbImport) - 1] = '\0';
		if ((sbVar != sbNil)		/* target name not nil */
		AND (vsbImport[0])		/* item name not nil */
		AND (! FSbCmp (sbVar, vsbImport))) /* target != item */
		{
		    visym++;
		    continue;
		}
		visdSaveSrch = visdSrch;
		visdSrch = FindModule (SbInCore (vsymCur->dimport.module),
				       visdSrch);
		if (visdSrch == isdNil)		/* not found */
		{
		    visdSrch = visdSaveSrch;
		}
		else
		{
		    visymSaveSrch = visym + 1;
		    visdBndSaveSrch = visdBndSrch;
		    SetUpSrch (visdSrch);
		    vfImportSrch = true;
		    isymBound = (visdSrch == visdBndSrch) ?
				vrgSd[visdSrch].sdIsymEnd :
				vrgSd[visdBndSrch].sdIsymStart - 1;
		    continue;  			/* don't increment visym */
		}
	    }
        /*
         * ENTRY statements with PARAMs are processed like FUNCTION
         * PARAMs.  Jump back up to FUNCTION handling for these.
         */
            if ((kind == K_ENTRY)
            AND (kind1 == K_NIL)                        /* FPARAM's wanted */
            AND (vsymCur->dentry.firstparam.word != DNTTNIL))
            {
                kind1 = K_FPARAM;
                isymFirst = visym + 1;     /* resume point if search fails */
                goto CheckParam;
            }

	    visym++;
	}  /* while */

    /*
     * Determine next section of the DNTT to search.  This is how we
     * skip over nested functions/modules entirely.  We also avoid jumping
     * to empty places between functions/modules.
     */
	if (visdSrch != visdBndSrch)
	{
	    while (true)
	    {
		visym = vrgSd[visdBndSrch].sdIsymEnd + 1;
		visdBndSrch = vrgSd[visdBndSrch].isdSibling;
		if (! visdBndSrch)
		{
		    visdBndSrch = visdSrch;
		    SetSym (visym);
		    break;
		}
		else if (visym != (vrgSd[visdBndSrch].sdIsymStart - 1))
		{
		    SetSym (visym);
		    break;
		}
	    }  /* while */
	}

    }  /* while */
    if (vfImportSrch)
    {
	SetSym (visymSaveSrch);
	visym = visymSaveSrch;
	visdSrch = visdSaveSrch;
	visdBndSrch = visdBndSaveSrch;
	vfImportSrch = false;
    }
    else
	break;				/* all done */
  } /* do forever */

    SetSym (isymFirst + 1);			/* failed to find anything */
    return (false);

#else  /* HPSYMTABII */

    long	isymFirst = visym;	/* note our starting point	*/

#ifdef XDB
    if (vfNoDebugInfo) {
       return(false);
    }
#endif

/*
 * CHECK FOR PARAM CHAIN:
 */

    if ((kind1 == K_FPARAM)			/* caller wants params	  */
    AND (visym >= isym0)			/* have a current symbol  */
    AND (! vsymCur->dblock.extension))		/* not at extension block */
    {
	while (true)				/* might follow the chain */
	{
	    kind = vsymCur->dblock.kind;
	    
	    if ((kind == K_FUNCTION)				/* at func   */
	    AND (vsymCur->dfunc.firstparam.word != DNTTNIL))	/* has chain */
	    {
                isymFunc = visym;
		isymNew = vsymCur->dfunc.firstparam.dnttp.index;
	    }
	    else if (kind == K_FPARAM)				/* at param   */
            {
                if (vsymCur->dfparam.nextparam.word != DNTTNIL) /* more chain */
	        {
		    isymNew = vsymCur->dfparam.nextparam.dnttp.index;
                }
                else
                {
                    SetSym (isymFunc);
                    break;
                }
	    }
	    else					/* wrong conditions */
	    {
		SetSym (isymFirst);			/* forget whole thing */
		break;
	    }
	    SetSym (isymNew);				/* assume it's param */

	    if ((sbVar == sbNil) OR FSbCmp (sbVar, NameFCurSym()))
		return (true);				/* we have a match */

	} /* while */
    } /* if */

    if (kind1 == K_FPARAM)
	kind1 =  K_NIL;				/* ignore params below */

/*
 * LOOK FORWARD FOR SYMBOL:
 */

    while (FNextSym (kind1, kind2, kind3, K_NIL, sbNil, nil,
		     kind4, kind5, K_FUNCTION, K_END))
    {
	if ((kind = vsymCur->dblock.kind) == K_FUNCTION)  /* START OF SCOPE */
	{
	    if (isymNest == isymNil)			/* not nested now   */
		isymNest =  visym;			/* save scope start */

	    continue;					/* go to next sym   */
	}
	if (kind == K_END)				/* END OF SOMETHING */
	{
	    isymBegin = vsymCur->dend.beginscope.dnttp.index;

	    if (isymNest == isymNil)			/* not nested now    */
	    {						/* end outer scope?  */
		isymEnd = visym;			/* we may come back  */
                if ((vsymCur->dend.endkind == K_FUNCTION) ||
                    (vsymCur->dend.endkind == K_MODULE)) {
		   SetSym (isymBegin);
                   break;
                }
		SetSym (isymEnd);			/* nope; come back   */
	    }
	    else {					/* we're nested now  */
		if (isymNest == isymBegin)		/* end of this scope */
		    isymNest =	isymNil;		/* not nested now    */
	    }
	    continue;					/* go to next sym */
	}

/*
 * CHECK NESTING, NAME, AND SUB-TYPE:
 */

	if (isymNest != isymNil)		/* we're nested now */
	    continue;				/* ignore other sym */

	if ((sbVar != sbNil)  AND ! FSbCmp (sbVar, NameFCurSym()))
	    continue;				/* name doesn't match */

	if ((kind == K_SVAR)  AND (vsymCur->dsvar.location == STATNIL))
	    continue;				/* no adr => does not exist */

	if ((kind == K_CONST) AND (vsymCur->dconst.locdesc  == LOC_PTR)
			      AND (vsymCur->dconst.location == STATNIL))
	    continue;				/* no adr => does not exist */

	return (true);				/* var passes all tests */

    } /* while */

    /* if at last global, do not want to run off end of DNTTs */
    SetSym ((isymFirst + 2 >= visymMax) ? isymFirst : isymFirst + 1);

    return (false);

#endif /* HPSYMTABII */

} /* FNextVar */


/***********************************************************************
 * A D R   F   E N D   O F   P R O C
 *
 * Map a procedure descriptor to its return adr (for setting breakpoint).
 * For HPSYMTABII, is called multiple times from IbpFSet() to retreive
 * all the EXIT SLTs for a given ipd.
 */

#ifdef HPSYMTABII
ADRT AdrFEndOfProc (ipd, isltCur)
    register int ipd;		/* procedure to map */
    long	*isltCur;	/* current SLT entry, for HPSYMTABII only */
#else
ADRT AdrFEndOfProc (ipd)
    register int ipd;		/* procedure to map */
#endif
				/* used to find all SLT_EXIT entries	  */
{

#ifdef HPSYMTAB
/*
 * Search forward in the SLT starting with the first entry (FUNC) for the
 * given ipd, until hitting the matching END SLT.  Return the address of
 * the last normal, non-prologue-code SLT entry seen.
 * This assumes all exits go through this last line!
 * FORTRAN ENTRY points must first be mapped to their enclosing routine.
 */

    long	isym;				/* starting FUNC entry	*/
    long	islt;				/* current SLT entry	*/
    ADRT	adr  = adrNil;			/* the result		*/

#ifdef INSTR
    vfStatProc[347]++;
#endif

    while (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd)	/* FORTRAN ENTRY */
	ipd--;
    isym = vrgPd[ipd].isym;			/* starting FUNC entry	*/
    islt = IsltFIsym (isym);			/* current SLT entry	*/

    SetNextSlt (islt);

    while (FNextSlt (SLT_NORMAL, SLT_END, SLT_NIL, SLT_NIL, SLT_NIL, SLT_NIL))
    {
	if (vsltCur->snorm.sltdesc == SLT_NORMAL)
	{
	    if (vsltCur->snorm.line != SLT_LN_PROLOGUE)
		adr = vsltCur->snorm.address;		/* remember address */
	}
	else {						 /* at an END SLT   */
	    SetSym (vsltCur->sspec.backptr.dnttp.index); /* check DNTT type */
	    /* see if it's the matching END */
	    if (vsymCur->dend.beginscope.dnttp.index == isym)
		return (adr);				/* return last adr */
	}
    }
    sprintf (vsbMsgBuf, (nl_msg(526, "Internal Error (IE526) (%d, %d)")),
        ipd, isym);
    Panic (vsbMsgBuf);

#endif /* HPSYMTAB */

#ifdef HPSYMTABII
/*
 * If first time routine called, search forward in the SLT starting with
 * the first entry (FUNC) for the given ipd, until hitting an EXIT SLT.
 * Otherwise, search forward starting with the SLT entry passed in.  After
 * the first time called, may not find additional EXIT SLTs, which is ok.
 */

    long	isym = vrgPd[ipd].isym;		/* starting FUNC entry	*/
    long	isltSave = *isltCur;		/* remember entry slt	*/
    ADRT	adr  = adrNil;			/* the result		*/

    if (*isltCur == islt0)
    {						/* get slt entry for FUNC */
	*isltCur = IsltFIsym (isym);	
        SetNextSlt (*isltCur);
    }

    while (FNextSlt (SLT_EXIT, SLT_END, SLT_NIL, SLT_NIL, SLT_NIL,
                     SLT_NIL, true))
    {
	if (vsltCur->snorm.sltdesc == SLT_EXIT)
	{
	    adr = vsltCur->snorm.address;		/* set address */
	    *isltCur = vislt;			        /* update entry SLT */
	    return (adr);				/* return adr of EXIT */
	}
        else {						/* at an END SLT */
            SetSym (vsltCur->sspec.backptr.dnttp.index);
	    /* see if it is the matching END */
            if (vsymCur->dend.endkind == K_FUNCTION)
		break;					/* all done */
        }
    }

    if (isltSave == islt0)	/* 1st time called, found no exits for proc */
    {
        sprintf (vsbMsgBuf, (nl_msg(548, "Internal Error (IE548) (%d, %d)")),
	         ipd, isym);
        Panic (vsbMsgBuf);
    }
   
    return (adr);

#endif /* HPSYMTABII */

} /* AdrFEndOfProc */


/***********************************************************************
 * F I L L   S S
 *
 * Fill the string cache starting from the given string space index.
 * Set global values.
 */

void FillSs (iss)
    register int iss;		/* string to set */
{
    long	 offset;	/* where to read file	*/
    long	 cRead;		/* bytes read in	*/

#ifdef INSTR
    vfStatProc[351]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if ((iss < 0) OR (iss >= vissMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(549, "Internal Error (IE549) (%d)")), iss);
	Panic (vsbMsgBuf);
    }

    offset = vcbSbFirst + iss;

    if (lseek (vfnSym, offset, 0) < 0L)
    {
	sprintf (vsbMsgBuf, (nl_msg(550, "Internal Error (IE550) (%d)")), offset);
	Panic (vsbMsgBuf);
    }

    if ((cRead = read (vfnSym, vsbCache, vcbSbCache)) < 0)
    {
	sprintf (vsbMsgBuf, (nl_msg(551, "Internal Error (IE551) (%d)")), vcbSbCache);
	Panic (vsbMsgBuf);
    }

    vissLo  = iss;
    vissLim = Min (vissMax, iss + cRead);
    vsbSs   = vsbCache;
    return;

} /* FillSs */

/***********************************************************************
 * I N I T   S S
 *
 * Initialize the string space cache.  If vcbSbCache == 0, read the
 * whole thing in.  This means that we don't really use the cacheing
 * mechanism, but we leave it for compatibility with systems where we
 * can't get away with this.
 */

void InitSs()
{

#ifdef INSTR
    vfStatProc[352]++;
#endif

#ifdef XDB
    if (vfNoDebugInfo) {
       return;
    }
#endif

    if (vcbSbCache == 0)
	vcbSbCache = vissMax;

    vsbCache = Malloc (vcbSbCache, true);
    FillSs (0);				/* force read first section */

    return;

} /* InitSs */


/***********************************************************************
 * S B	 I N   C O R E
 *
 * Given a value which is either an iss (index into the object file
 * string space) or an in-memory-string pointer, either read the string
 * into memory or just make the pointer usable.	 Uses an ugly kludge to
 * tell the difference; see sym.h for more.
 *
 * This routine is neither export nor local; it is a macro for non-BSD,
 * non-HPSYMTAB(II) versions.
 */

char * SbInCore (sb)
    char	 *sb;		/* iss to read in or string ptr to fix */
{
    register long iss = (long) sb;	/* current iss */

#ifdef INSTR
    vfStatProc[353]++;
#endif

    if (iss == -1)			/* nil pointer */
	return (sbNil);

#ifdef FOCUS
    if (iss & (~bitsIssMax))		/* a debugger string */
	return ((char *) iss);
#else
    if (iss & bitHigh)
	return ((char *) (iss & ~bitHigh));
#endif

    SetIss (iss);			/* a string space string */
    return (vsbSs);

} /* SbInCore */


/***********************************************************************
 * S E T   I S S
 *
 * Make sure the string starting at the given iss is in cache, e.g. the
 * WHOLE string has been read in, and set global values.  If it has to
 * read a new cache, it starts at the pos of the string and reads one
 * cachefull without checking that the complete string fits in the cache.
 */

void SetIss (iss)
    register long	iss;		/* string to set */
{
    long	issSave = iss;		/* note start		*/
    register char	*sbTest;	/* for looking for end	*/
    long	offset;			/* where to read file	*/
    long	cRead;			/* bytes read in	*/

#ifdef INSTR
    vfStatProc[354]++;
#endif

    if ((iss < 0) OR (iss >= vissMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(552, "Internal Error (IE552) (%d)")), iss);
	Panic (vsbMsgBuf);
    }

/*
 * START IS IN CACHE; check for end:
 */

    if ((iss >= vissLo) AND (iss < vissLim))
    {
	sbTest = vsbCache + (iss - vissLo);

	while ((iss < vissLim) AND (*sbTest != chNull))
	{
	    sbTest++;
	    iss++;
	}

	if ((*sbTest == chNull)	AND (iss < vissLim))	/* found end */
	{
	    iss = issSave;				/* reset pos */
	    vsbSs = vsbCache + (iss - vissLo);	/* set start */
	    return;
	}					/* else keep going */
	iss = issSave;				/* reset pos */
    }

/*
 * WHOLE STRING NOT IN CACHE; read it into start of cache and set values:
 */

    FillSs (iss);
    return;

} /* SetIss */


/***********************************************************************
 * V   T   I N   C O R E
 *
 * Make sure the whole item starting at the given iss is in cache
 * and set global values.  If it has to
 * read a new cache, it starts at the pos of the item and reads one
 * cachefull without checking that the complete item fits in the cache.
 */

char *VTInCore (iss, cb)
    register long iss;		/* VT index to set */
    long	  cb;		/* length of item in bytes */
{
    register long issEnd;		/* index of end of item */

#ifdef INSTR
    vfStatProc[355]++;
#endif

    if ((iss < 0) OR (iss >= vissMax))
    {
	sprintf (vsbMsgBuf, (nl_msg(553, "Internal Error (IE553) (%d)")), iss);
	Panic (vsbMsgBuf);
    }

    issEnd = iss + cb - 1;
    if ((iss >= vissLo) AND (iss < vissLim) AND (issEnd < vissLim))
	vsbSs = vsbCache + (iss - vissLo);	/* set start */
    else
	FillSs (iss);			/* read item into cache */
    return (vsbSs);

}  /* VTInCore */

#ifdef DEBREC
extern int xdbenable;
#endif

/***********************************************************************
 * I N I T   F D   P D
 *
 * This monster initializes the file and procedure descriptors (quick
 * reference arrays, vrgFd and vrgPd) and related values.  Non-HPSYMTAB
 * versions read the symbol table twice, once to see how much memory to
 * get, then to copy useful data into memory.  HPSYMTAB reads the SLT
 * only once and uses expandable arrays to store the data.  InitFdPd stops
 * after seeing the special procedure
 * "_end_()".  (HPSYMTAB notes the proc but does not stop there).
 *
 * Since the bulk of init time is spent here and it can be long, goes
 * to some trouble to let the user know it's running, using direct,
 * unbuffered, write(2)s of "busy" chars, based on user procs seen.
 */

void InitFdPd()
{
#ifdef HPSYMTABII
    int i;
    struct PXDB_header pxdbh;
    register pFDR fd;			/* for working with fds */
    char	*sbTemp;		/* for name processing	*/
    char	*sb;			/* misc junk          	*/

    if (vfNoDebugInfo) {
       vrgPd = NULL;
       vipdMac = 0;
       vipd = ipdNil;

       vrgFd = (pFDR) Malloc(2 * cbFDR, true);
       vifdMac = 0;

       vrgMd = NULL;
       vimdMac = 0;

       visymGlobal = 0;
#ifdef DEBREC
	/* No debug records, therefore shut off */
	xdbenable = 0;
#endif
    }
    else {
       lseek(vfnSym, vipxdbheader, 0);
       read(vfnSym, &pxdbh, sizeof(struct PXDB_header));
       lseek(vfnSym, vipdfdmd, 0);
       vrgPd = (pPDR) Malloc (pxdbh.pd_entries * cbPDR, true);
       read(vfnSym, vrgPd, pxdbh.pd_entries *cbPDR);
       vrgFd = (pFDR) Malloc ((pxdbh.fd_entries + 2) * cbFDR, true);
       read(vfnSym, vrgFd, pxdbh.fd_entries *cbFDR);
       vrgMd = (pMDR) Malloc (pxdbh.md_entries * cbMDR, true);
       read(vfnSym, vrgMd, pxdbh.md_entries *cbMDR);

       visymGlobal = pxdbh.globals;

       for (i=0; i < pxdbh.pd_entries; i++) 
       {
          sbTemp	= SbInCore (vrgPd[i].sbAlias);	/* save alias */
          vrgPd[i].sbAlias = Malloc (strlen (sbTemp) + 2, true);
          strcpy (vrgPd[i].sbAlias, sbTemp);

          sbTemp	= SbInCore (vrgPd[i].sbProc);	/* save name */
          vrgPd[i].sbProc	= Malloc (strlen (sbTemp) + 2, true);
          strcpy (vrgPd[i].sbProc, sbTemp);
       }

       vipdMac = pxdbh.pd_entries;		/* set # of entries in vrgPd */
       vipd = ipdNil;

       for (i=0; i < pxdbh.fd_entries; i++) 
       {
          sbTemp = SbInCore (vrgFd[i].sbFile);
#ifdef HPE
          if (sb = strrchr(sbTemp, '/'))	/* remove Unix directory info */
	      sbTemp = sb+1;
          if ((sb = strchr(sbTemp, '.')) &&	/* remove Unix C suffix info  */
	      (*(sb+1) == 'c') &&
	      (*(sb+2) == '\0'))
	      *sb = '\0';
#endif
          vrgFd[i].sbFile = Malloc (strlen (sbTemp) + 2, true);
          strcpy (vrgFd[i].sbFile, sbTemp);
       }

       vifdMac = pxdbh.fd_entries;		/* set # of entries in vrgFd */

       for (i=0; i < pxdbh.md_entries; i++) 
       {
          sbTemp	= SbInCore (vrgMd[i].sbAlias);	/* save alias */
          vrgMd[i].sbAlias = Malloc (strlen (sbTemp) + 2, true);
          strcpy (vrgMd[i].sbAlias, sbTemp);

          sbTemp     = SbInCore (vrgMd[i].sbMod);
          vrgMd[i].sbMod = Malloc (strlen (sbTemp) + 2, true);
          strcpy (vrgMd[i].sbMod, sbTemp);
       }

       vimdMac = pxdbh.md_entries;		/* set # of entries in vrgFd */


#ifdef DEBREC
  	if( (vipdMac == 0) && (visymGlobal == 0)){
          	printf (" Global ddbmerge records available\n");
	} else {
       		if (!vfprntipdifd) {
          		printf ((nl_msg(226, " Procedures: %d, ")), vipdMac);
          		printf ((nl_msg(235, " Files: %d, ")), vifdMac);
          		printf ((nl_msg(235, " Globals: %d\n")), visymGlobal);
       		}
	}
#else
       if (!vfprntipdifd) {
          printf ((nl_msg(226, "Procedures: %d\n")), vipdMac);
          printf ((nl_msg(235, "Files: %d\n")), vifdMac);
       }
#endif
    }

    vifdTemp = vifdMac++;		/* temp fd is last - 1 */
    vifdNil  = vifdMac++;		/* nil	fd is last one */
    vifd = vifdNil;

    visymLo  = 0;			/* initialize symbol cache ptrs. */
    visymLim = -1;

    fd = vrgFd + vifdTemp;	/* vifdTemp is used for "other" files */
    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = "";
    fd->fHasDecl = false;
    fd->fWarned	 = true;	 /* assume warning here isn't important */

    fd = vrgFd + vifdNil;	/* vifdNil is used as a "catchall" */
    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = "no source file";
    fd->fHasDecl = false;
    fd->fWarned	 = true;

#else /* not HPSYMTABII */

    FLAGT	fEndSeen;	/* encountered end.o?	*/
    short	ipd = 0;	/* pd, to note in fd	*/

    register pFDR fd;		/* for working with fds */
    register pPDR pd;		/* for working with pds */

    char	*sbTemp;	/* for name processing	*/

    int		cBusy	  = 0;	/* procs for this note	*/
    int		cBusyLine = 0;	/* notes for this line	*/
#define		cBusyMax  20	/* procs per busy note	*/
#define		cBusyLMax 50	/* notes per line	*/
#define		sbBusy    "."	/* string to display	*/
    int		fnStdout = fileno (stdout);

    FLAGT	fSrcSeen = false;	/* new SRCFILE entry seen	*/
    FLAGT	fSrcOpen = false;	/* currently processing SRCFILE */
    FLAGT	fFNormalSeen = false;	/* NORMAL entry for FUNC seen	*/
    DNTTPOINTER	iFuncDNTT;		/* DNTT ptr to FUNCTION entry-- */
					/* don't have to backchain from */
					/* END entry if have this value */
    long	ipdInit;		/* pd array initial alloc */
    long	ifdInit;		/* fd array initial alloc */
    long	ipdIncr;		/* pd array incr alloc  */
    long	ifdIncr;		/* fd array incr alloc  */
    char	sbFdNameSeen[MAXNAMLEN+1];/* name of SRCFILE seen -- may  */
					/* not have any executable code */
    long	isymFd;			/* SRCFILE visym value		*/
    long	ipdNormal;		/* last pd entry with bp addr	*/

#ifdef INSTR
    vfStatProc[356]++;
#endif

/*
 * Make initial allocation of arrays
 */
    ipdInit = Max (3, (long) (visltMax * 0.05));  /* initial alloc's */
    ifdInit = Max (3, (long) (visltMax * 0.03));
    ipdIncr = Max (2, (long) (visltMax * 0.05));  /* size of incr alloc's */
    ifdIncr = Max (2, (long) (visltMax * 0.03));
    vifdMac = ifdInit;
    vipdMac = ipdInit;
    vrgFd = (pFDR) Malloc (vifdMac * cbFDR, true);
    vrgPd = (pPDR) Malloc (vipdMac * cbPDR, true);
    vifd = 0;			/* index of next fd entry	*/
    vipd = 0;			/* index of next pd entry	*/
    fEndSeen = false;
    ipdNormal = -1;		/* haven't filled any bp addrs yet */
    fflush (stdout);		/* so write(2)s are in order	*/

/*
 * Process entire SLT
 */
    visymLo  = 0;		/* initialize symbol cache ptrs. */
    visymLim = -1;
    visltLo  =	0;
    visltLim = -1;
    vislt = -1;			/* kludge to get search started on 1st SLT */

    while (FNextSlt (SLT_SRCFILE, SLT_FUNCTION, SLT_NORMAL, SLT_END, SLT_ENTRY,
			SLT_NIL))
    {
	switch (vsltCur->sspec.sltdesc)
	{
	case SLT_FUNCTION:
	case SLT_ENTRY:
	    /*
	     * Expand pd array if necessary
	     */
		if (vipd+1 >= vipdMac)
		{
		    vipdMac += ipdIncr;
    		    vrgPd = (pPDR) Realloc (vrgPd, vipdMac * cbPDR);
		}
			
	    /*
             * Read function information from DNTT and put in pd array
	     */
		iFuncDNTT = vsltCur->sspec.backptr;
		SetSym (iFuncDNTT.dnttp.index);
		pd = &(vrgPd[vipd]);
	        pd->isym = visym;			/* its symbol */
	        pd->adrStart = vsymCur->dfunc.entry;	/* adr bounds	 */
		if (vsltCur->sspec.sltdesc == SLT_ENTRY)
		    pd->adrEnd = pd->adrStart;
		else
	       	    pd->adrEnd = vsymCur->dfunc.nextaddr - 1;  /* note end */
	        sbTemp	= SbInCore (vsymCur->dfunc.alias);	/* save alias */
	        pd->sbAlias = Malloc (strlen (sbTemp) + 2, true);
	        strcpy (pd->sbAlias, sbTemp);
	        sbTemp = SbInCore (vsymCur->dfunc.name);	/* save name */
	        pd->sbProc = Malloc (strlen (sbTemp) + 2, true);
	        strcpy (pd->sbProc, sbTemp);

		if (vsltCur->sspec.sltdesc == SLT_FUNCTION)
		    fFNormalSeen = false;
		vipd++;

		if ((! strcmp (pd->sbProc, "_end_"))
		   OR (! strcmp (pd->sbAlias, "_end_")))
		{
		    fEndSeen = true;
		}

		if (++cBusy >= cBusyMax)		/* time to notify */
		{
                    write (fnStdout, sbBusy, 1);        /* force out one char */
		    cBusy = 0;

		    if (++cBusyLine >= cBusyLMax)	/* time to newline */
		    {
                        write (fnStdout, "\n", 1);      /* force out one char */
			cBusyLine = 0;
		    }
		}
		break;

	case SLT_SRCFILE:
	    /*
	     * Information on SRC files is not put in the FD array unless
	     * there are executable lines in the file.  Hence, defer 
	     * allocating an fd entry until seeing an NORMAL SLT entry.
	     * Remember source file name in case we need it later.
	     * This scheme assumes the compilers always emit at least
	     * one NORMAL entry for every FUNCTION.
	     */
	    /*
	     * Get name from DNTT entry
	     */
		isymFd = vsltCur->sspec.backptr.dnttp.index;
		SetSym (isymFd);
	        sbTemp = SbInCore (vsymCur->dsfile.name);	/* save name */
	        strcpy (sbFdNameSeen, sbTemp);

	    /*
	     * Check if same source file as currently processing
	     */
		if (fSrcOpen && (! strcmp (vrgFd[vifd-1].sbFile, sbFdNameSeen)))
		    fSrcSeen = false;
		else
		    fSrcSeen = true;
		break;

	case SLT_END:
	    /*
	     * If the END terminates a FUNCTION and a new SRCFILE has been
	     * seen (but not started), close off the current SRCFILE
	     */
		if (fSrcSeen)		/* Do it this way for efficiency */
		{
		    SetSym (vsltCur->sspec.backptr.dnttp.index);
		    if (vsymCur->dend.beginscope.word == iFuncDNTT.word)
		    {
			pd = &(vrgPd[vipd-1]);
			while (pd->adrStart == pd->adrEnd)
			    pd--;		/* skip back over ENTRY pd's */
			fd->adrEnd = pd->adrEnd;
			fSrcOpen = false;
		    }
		}	/* do nothing if NOT new SRCFILE or END NOT -> FUNC */
		break;

	case SLT_NORMAL:
	    /*
	     * If a new SRCFILE has been seen, close off the current one
	     * and start a new one
	     */
		if (fSrcSeen)
		{
		    if (fSrcOpen)
		    {
		    /*
		     * fNormalSeen == false iff this is first NORMAL following
		     * a FUNCTION.  If starting new function, end address of
		     * last source file = end address of previous function.
		     * If not starting new function, end address of last file
		     * = NORMAL address - 1.
		     */
			if (fFNormalSeen)
			{
		            fd->adrEnd = vsltCur->snorm.address-1;
			}
			else
			{
			    pd = &(vrgPd[vipd-2]);
			    while (pd->adrStart == pd->adrEnd)
			        pd--;		/* skip back over ENTRY pd's */
			    fd->adrEnd = pd->adrEnd;
			}
			fSrcOpen = false;
		    }

		/*
		 * Save file info in FD quick reference array.  Expand array
		 * if necessary.
		 */
		    if (vifd+3 >= vifdMac)  /* save room for tmp file entries */
		    {
		        vifdMac += ifdIncr;
    		        vrgFd = (pFDR) Realloc (vrgFd, vifdMac * cbFDR);
		    }

		    fd = &(vrgFd[vifd]);
		    ipd = vipd-1;
		    while (vrgPd[ipd].adrStart == vrgPd[ipd].adrEnd)
		        ipd--;		/* skip back over ENTRY pd's */
		    fd->adrStart = (fFNormalSeen? vsltCur->snorm.address:
						  vrgPd[ipd].adrStart);
				/* if first SLT NORMAL of function, use */
				/* start addr of function, else use line */
				/* addr					*/
		    fd->isym	 = isymFd;	/* its symbol		*/
		    fd->ipd	 = ipd;		/* its first proc	*/
		    fd->ilnMac	 = ilnNil;	/* no line arrays yet	*/
		    fd->rgLn	 = (uint *) 0;
		    fd->fHasDecl = true;	/* goes false if none	*/
		    fd->fWarned	 = false;
		    fd->sbFile = Malloc (strlen (sbFdNameSeen) + 2, true);
		    strcpy (fd->sbFile, sbFdNameSeen);

		    fSrcSeen = false;
		    fSrcOpen = true;
		    vifd++;
		}
		fFNormalSeen = true;

	     /*
	      * Save breakpoint address info in all unfilled Pd entries.
	      * The only time we fill more than one is for empty ENTRY points.
	      */

		for (ipd = ipdNormal + 1; ipd < vipd; ipd++)
		    vrgPd[ipd].adrBp = vsltCur->snorm.address;
		ipdNormal = vipd - 1;
		    
		break;

	}  /* switch */
    }  /* while */

/*
 * Close off last source file
 */
    if (fSrcOpen)
    {
	pd = &(vrgPd[vipd-1]);
	while (pd->adrStart == pd->adrEnd)
	    pd--;		/* skip back over ENTRY pd's */
	fd->adrEnd = pd->adrEnd;
    }

    vifdMac = vifd;
    vipdMac = vipd;
    vifd = vifdNil;
    vipd = ipdNil;
    vifdTemp = vifdMac++;		/* temp fd is last - 1 */
    vifdNil  = vifdMac++;		/* nil	fd is last one */

/*
 * For HPSYMTAB we don't automatically decrement vifdMac for *rt0.o because
 * it might not be debuggable.	If it DOES appear in the symbol table we skip
 * it later and "waste" just one fd entry in memory.
 */

/*
 * DISPLAY AND CHECK STATUS:
 */

    cBusy = 0;

    if (cBusyLine)			/* finish last line */
    {
	write (fnStdout, "\n", 1);	/* force out one char */
	cBusyLine = 0;
    }

    if (vipdMac == 0)
    {
	sprintf (vsbMsgBuf, (nl_msg(445, "It appears that there's no debugging information in %s")), vsbSymfile);
	UError (vsbMsgBuf);
    }

#ifndef XDB
    printf ((nl_msg(226, "Source files: %4d\n")), vifdMac - 2);
#endif
    printf ((nl_msg(225, "Procedures:   %4d\n")), vipdMac);

    if (!fEndSeen)
#ifdef XDB
	printf ((nl_msg(227, "WARNING:  \"xdbend.o\" was not linked with this program\n")));
#else
	printf ((nl_msg(227, "WARNING:  \"end.o\" was not linked with this program\n")));
#endif

/*
 * SET SPECIAL fd ENTRIES:
 */

    fd = vrgFd + vifdTemp;	/* vifdTemp is used for "other" files */

    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = sbNil;
    fd->fHasDecl = false;
    fd->fWarned	 = true;	 /* assume warning here isn't important */

    fd = vrgFd + vifdNil;	/* vifdNil is used as a "catchall" */

    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = vsbnosourcefile;
    fd->fHasDecl = false;
    fd->fWarned	 = true;
#endif  /* not HPSYMTABII */

    return;

} /* InitFdPd */

#ifdef S200
/***********************************************************************
 * I N I T   N O   S Y M S
 *
 * Initialize global variables in the case where no debug information
 * is present.
 */

void InitNoSyms()
{
    register pFDR fd;

#ifdef INSTR
    vfStatProc[357]++;
#endif

    vipdMac = 0;
    vipd = 0;
    visdMac = 0;
    visym = -1;
    visltLo = 0;
    visltLim = 0;
    visltMax = 0;
    vislt = -1;
    vsltCur = NULL;
    vsymCur = NULL;
/*
 * Put single null byte in string cache
 */
    vcbSbCache = 1;
    vsbCache = Malloc (vcbSbCache, true);
    *vsbCache = '\0';
    vsbSs = vsbCache;
    vissLo = 0;
    vissLim = 1;
    vissMax = 1;

/*
 * Put 2 file descriptor entries in file list -- tempfile and nil
 */
    vifdMac = 2;
    vrgFd = (pFDR) Malloc (vifdMac * cbFDR, true);
    vifdTemp = 0;
    vifdNil = 1;
    vifd = vifdNil;

/*
 * SET SPECIAL fd ENTRIES:
 */

    fd = vrgFd + vifdTemp;	/* vifdTemp is used for "other" files */

    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = sbNil;
    fd->fHasDecl = false;
    fd->fWarned	 = true;	 /* assume warning here isn't important */

    fd = vrgFd + vifdNil;	/* vifdNil is used as a "catchall" */

    fd->adrStart = 0;
    fd->adrEnd	 = 0;
    fd->isym	 = isym0;
    fd->ipd	 = ipdNil;
    fd->ilnMac	 = ilnNil;
    fd->rgLn	 = (uint *) 0;
    fd->sbFile	 = vsbnosourcefile;
    fd->fHasDecl = false;
    fd->fWarned	 = true;

    return;

}  /* InitNoSyms */
#endif


/***********************************************************************
 * N A M E   F	 C U R	 S Y M
 *
 * Return the name string for the current (named) symbol.
 * Does NOT know about aliases!
 */

char * NameFCurSym ()
{
    register long iss;		/* where to find the name in VT */

#ifdef INSTR
    vfStatProc[358]++;
#endif

    switch (vsymCur->dblock.kind)
    {

	default:	 sprintf (vsbMsgBuf,
			     (nl_msg(554, "Internal Error (IE554) (%d, %d)")),
			     vsymCur->dblock.kind, visym);
			 Panic (vsbMsgBuf);

	case K_SRCFILE:	 iss = vsymCur->dsfile.name ;	break;
	case K_MODULE:	 iss = vsymCur->dmodule.name;	break;
	case K_FUNCTION: iss = vsymCur->dfunc.name  ;	break;
	case K_ENTRY:	 iss = vsymCur->dentry.name ;	break;
#ifdef HPSYMTABII
	case K_IMPORT:	 iss = vsymCur->dimport.module;	break;
#else
	case K_IMPORT:	 iss = vsymCur->dimport.item;	break;
#endif
	case K_LABEL:	 iss = vsymCur->dlabel.name ;	break;
	case K_FPARAM:	 iss = vsymCur->dfparam.name;	break;
	case K_SVAR:	 iss = vsymCur->dsvar.name  ;	break;
	case K_DVAR:	 iss = vsymCur->ddvar.name  ;	break;
	case K_CONST:	 iss = vsymCur->dconst.name ;	break;
	case K_TYPEDEF:	 iss = vsymCur->dtype.name  ;	break;
	case K_TAGDEF:	 iss = vsymCur->dtag.name   ;	break;
	case K_MEMENUM:	 iss = vsymCur->dmember.name;	break;
	case K_FIELD:	 iss = vsymCur->dfield.name ;	break;
#ifdef HPSYMTABII
	case K_WITH:	 iss = vsymCur->dwith.name  ;	break;
	case K_COMMON:	 iss = vsymCur->dcommon.name;	break;
#endif
    }
    if (iss == VTNIL)			/* no name */
	return (sbNil);

    return (SbInCore (iss));

} /* NameFCurSym */


/***********************************************************************
 * F   N A M E   C M P
 *
 * Compare the name of the current (named) symbol with the given name
 * using the given Cmp() function.  If the name doesn't match, but we
 * have a func or entry symbol, also check the alias.
 */

FLAGT FNameCmp (sbHit, Cmp)
    char	*sbHit;		/* pattern to look for	  */
    FLAGT	(* Cmp)();	/* compare routine to use */
{
    long	kind;		/* kind of symbol */

#ifdef INSTR
    vfStatProc[359]++;
#endif

    if ((*Cmp) (sbHit, NameFCurSym()))
	return (true);

    if ((kind = vsymCur->dblock.kind) == K_FUNCTION)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dfunc.alias)));

    if (kind == K_ENTRY)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dentry.alias)));

#ifdef HPSYMTABII
    if ((kind = vsymCur->dblock.kind) == K_MODULE)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dmodule.alias)));

    if (kind == K_COMMON)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dcommon.alias)));
#endif

    return (false);

} /* FNameCmp */


#if (FOCUS || SPECTRUM)
/***********************************************************************
 * S B	 F   I N P
 *
 * Read a name from the linker's name pool (may not be the whole name).
 * Skip one leading "_", if any, then return up to cbProcMax chars.
 *
 * This is very crude and inefficient compared to routines which read
 * and cache other tables from the disc.  Some filesystems do some
 * caching for us so it may not matter.
 */

char * SbFInp (inp)
    long	inp;			/* where to read the name */
{
#ifdef SPECTRUM
    char *sbRet = vrgnp + inp;
    return (sbRet);
#else
    static char sb [cbProcMax + 2];	/* where to save the name */
    char	*sbRet = sb;		/* part to return	  */

#ifdef INSTR
    vfStatProc[360]++;
#endif

    if (lseek (vfnSym, vcbNpFirst + inp, 0) < 0)	/* seek start of name */
    {
	sprintf (vsbMsgBuf, (nl_msg(555, "Internal Error (IE555) (%d)")), vcbNpFirst + inp);
	Panic (vsbMsgBuf);
    }

    if (read (vfnSym, sb, cbProcMax + 1) < 0)	/* OK if read less than want */
    {
	sprintf (vsbMsgBuf, (nl_msg(556, "Internal Error (IE556) (%d)")), cbProcMax);
	Panic (vsbMsgBuf);
    }

    sbRet += (*sbRet == '_');		/* skip leading '_'   */
    sbRet [cbProcMax] = 0;		/* insure ending null */
    return (sbRet);
#endif

} /* SbFInp */

#endif  /* FOCUS || SPECTRUM */

#ifdef S200
/***********************************************************************
 * C O M P   L S T   A D R
 *
 * Compare LST addresses -- called by qsort(3)
 */

static long CompLSTAdr (pPd1, pPd2)
    register pLSTPDR pPd1;	/* first item to be compared	*/
    register pLSTPDR pPd2;	/* second item to be compared	*/
{
#ifdef INSTR
    vfStatProc[361]++;
#endif

    if (pPd1->adrStart < pPd2->adrStart)
	return -1;
    else if (pPd1->adrStart == pPd2->adrStart)
	return 0;
    else
	return 1;

}  /* CompLSTAdr */


/***********************************************************************
 * I N I T   L S T   P D
 *
 * Initialize the LST Procedure quick reference table 
 */

void InitLSTPd ()
{
#define PDMAX 100
#define CACHEMAX 1024
#define CBLSTR ((long)(sizeof(struct nlist_)))

    struct pdTemp {
		struct pdTemp *nxt;	/* pointer to next block	*/
		LSTPDR	pd[PDMAX];	/* elements of this block	*/
		} *pPdHead;		/* ptr to first one		*/
    register struct pdTemp  *pPdCurr;	/* ptr to current one		*/
    struct pdTemp	*pPdNew;	/* ptr to new one (malloc'd)	*/
    register long	iPdMax;		/* total proc entries		*/
    register long	iPd;		/* next pd entry in curr block	*/
    register struct nlist_ *plstCur;	/* where to stuff LST symbol	*/
    register long cbSeek;		/* place in file		*/
    long	cbRead;			/* number bytes read in		*/
    register long type;			/* type of LST entry		*/
    long	length;
    long	CompLSTAdr ();
    register long icacheLo;		/* low LST offset in cache	*/
    register long icacheHi;		/* high LST offset + 1 in cache */
    register char *pcache;		/* pointer to LST cache		*/

#ifdef INSTR
    vfStatProc[362]++;
#endif

/*
 * Allocate initial temporary array
 */

    pPdHead = (struct pdTemp *) Malloc (sizeof (struct pdTemp), true);
    pPdCurr = pPdHead;
    iPdMax = 0;
    iPd = 0;
    pPdHead->nxt = NULL;

/*
 * Allocate LST cache
 */

    pcache = (char *) Malloc (CACHEMAX, true);
    icacheLo = -1;
    icacheHi = -1;		/* empty so far */

/*
 * SCAN THE LST, filling the temporary arrays:
 */

    for (cbSeek = vcbLstFirst; cbSeek < vcbLstFirst + vilstMax; )
    {
	if ((cbSeek + CBLSTR) >= icacheHi)	/* next LSTR not in cache */
	{
    	    if (lseek (vfnSym, cbSeek, 0) < 0)
	    {
		sprintf (vsbMsgBuf, (nl_msg(557, "Internal Error (IE557) (%d)")), cbSeek);
		Panic (vsbMsgBuf);
	    }

	    if ((cbRead = read (vfnSym, pcache, CACHEMAX)) < CBLSTR)
	    {
	    	sprintf (vsbMsgBuf, (nl_msg(558, "Internal Error (IE558) (%d)")), cbRead);
	    	Panic (vsbMsgBuf);
	    }

	    icacheLo = cbSeek;
	    icacheHi = icacheLo + cbRead;
	}

	plstCur = (struct nlist_ *) (pcache + (cbSeek - icacheLo));

	type = plstCur->n_type & 077; 	/* only want lowest 6 bits	*/
	length = plstCur->n_length;
	if (type & (long) EXTERN)
	{
	    if (iPd == PDMAX)		/* allocate new temp array	*/
	    {
    		pPdNew = (struct pdTemp *) Malloc (sizeof (struct pdTemp),
						   true);
		pPdNew->nxt = NULL;
		pPdCurr->nxt = pPdNew;
		pPdCurr = pPdNew;
		iPd = 0;
	    }
		
	/*
	 * Copy start address of proc to proc descriptor structure.  I must
	 * do it this round about way because plstCur->n_value may not be
	 * on a word boundary and a direct assign will generate code that
	 * assumes it is -- leading to a bus error.
	 */
	    {
		char *target = (char *) (&(pPdCurr->pd[iPd].adrStart));
		char *src = (char *) (&(plstCur->n_value));
		long i;
		for (i=0; i<4; i++)
		    *target++ = *src++;
	    }
	    pPdCurr->pd[iPd].sbProc = Malloc (length+1, true);

	    if ((cbSeek + CBLSTR + length) >= icacheHi)
	    {						/* name not in cache */
    	        if (lseek (vfnSym, cbSeek + CBLSTR, 0) < 0)
		{
		    sprintf (vsbMsgBuf, (nl_msg(557, "Internal Error (IE557) (%d)")),
			     cbSeek + CBLSTR);
		    Panic (vsbMsgBuf);
		}

	        if ((cbRead = read (vfnSym, pcache, CACHEMAX)) < length)
		{
	    	    sprintf (vsbMsgBuf, (nl_msg(558, "Internal Error (IE558) (%d)")),
			     cbRead);
	    	    Panic (vsbMsgBuf);
		}

	        icacheLo = cbSeek + CBLSTR;
	        icacheHi = icacheLo + cbRead;
	    }

	    strncpy (pPdCurr->pd[iPd].sbProc,
		     (char *) (pcache + (cbSeek + CBLSTR - icacheLo)),
		     length);			/* copy name from cache */
	    pPdCurr->pd[iPd].sbProc[length] = '\0';  /* terminating null */
	    iPd++;
	    iPdMax++;
	}
	cbSeek += CBLSTR + length;
    } /* for */

   /*
    * Free cache
    */

    Free (pcache);

   /*
    * Allocate actual LST Pd array and copy proc entries into it
    */

    viLSTPdMax = iPdMax;
    vrgLSTPd = (pLSTPDR) Malloc (viLSTPdMax * cbLSTPDR, true);
    pPdCurr = pPdHead;
    iPd = 0;
    for (iPdMax = 0; iPdMax < viLSTPdMax; iPdMax++)
    {
	if (iPd == PDMAX)
	{
	    pPdNew = pPdCurr;		/* Save value for discarding	*/
	    pPdCurr = pPdCurr->nxt;
	    Free (pPdNew);		/* Free temporary array		*/
	    iPd = 0;
	}
	vrgLSTPd[iPdMax] = pPdCurr->pd[iPd++];	/* Copy values to actual  */
    }

   /*
    * Sort the array so start addresses are in increasing order
    */

    qsort (vrgLSTPd, viLSTPdMax, cbLSTPDR, CompLSTAdr);
    return;

} /* InitLSTPd */


#ifdef S200BSD
/*****************************************************************************
 * U A R E A   F   L S T
 *
 * Read the uarea offset from the system file ( "/hp-ux" )
 */

long UareaFLst ()
{
    struct nlist nl[2];

#ifdef INSTR
    vfStatProc[363]++;
#endif

    nl[0].n_name = "_u";
    nl[1].n_name = (char *) 0;

    nlist ("/hp-ux", nl);

    if (nl[0].n_value == 0)
        Panic ((nl_msg(559, "Internal Error (IE559)")));
    else
	return (nl[0].n_value);

}  /* UareaFLst */
#endif  /* S200BSD */
#endif  /* S200 */

#ifdef HPSYMTAB
long	cBusy;		/* procs for this note	*/
long	cBusyLine;	/* notes for this line	*/
#define		cBusyMax  20	/* procs per busy note	*/
#define		cBusyLMax 50	/* notes per line	*/
#define		sbBusy    "."	/* string to display	*/
long	fnStdout;

/************************************************************************
 * S C O P E
 *
 * Create all scope descriptor records for the current scope
 */

static void Scope (isdParent, isymRet)
    register long isdParent;	/* SD index of parent -- current scope	*/
    long	  isymRet;	/* return if END backptr matches this	*/
{
    long	  isdSibLast;	/* SD inx of last sibling in scope	*/
    char	 *sbTemp;	/* name pointer				*/
    ushort	  isymTy;	/* type of DNTT pointer			*/
    register long isdCur;	/* current sd entry			*/
    register long ipd;		/* corresponding Pd entry for FUNC's	*/
    register pSDR sd;		/* for quick reference			*/

#ifdef INSTR
    vfStatProc[364]++;
#endif

    isdSibLast = isdNil;	/* no siblings yet			*/

    while (true)
    {
	visym++;		/* advance to next DNTT record		*/
	if (visym >= visymMax)
	    return;		/* done with entire DNTT		*/
	else
	    SetSym (visym);

	switch (vsymCur->dblock.kind)
	{
	case K_MODULE:
	case K_FUNCTION:

	  /*
	   * Expand sd array if necessary
	   */
	    visdMac++;
	    if (visdMac >= visdMax)
	    {
		visdMax += visdIncr;
		vrgSd = (pSDR) Realloc (vrgSd, visdMax * cbSDR);
	    }

	  /*
	   * Set up scope entry
	   */
	    isdCur = visdMac;
	    sd = &(vrgSd[isdCur]);
	    isymTy = vsymCur->dblock.kind;
	    sd->sdTy = isymTy;

	    sbTemp = SbInCore (vsymCur->dfunc.name);
	    sd->sdName = Malloc (strlen (sbTemp) + 2, true);
	    strcpy (sd->sdName, sbTemp);
	    if (isymTy == K_FUNCTION)
	    {
	        ipd = IpdFAdr (vsymCur->dfunc.entry);
	        sd->sdIpd = ipd;
	        vrgPd[ipd].isd = isdCur;

	    /*
	     * Put out the dots, so the user knows we're busy
	     */
		if (++cBusy >= cBusyMax)		/* time to notify */
		{
                    write (fnStdout, sbBusy, 1);        /* force out one char */
		    cBusy = 0;

		    if (++cBusyLine >= cBusyLMax)	/* time to newline */
		    {
                        write (fnStdout, "\n", 1);      /* force out one char */
			cBusyLine = 0;
		    }
		}
	    }

	    sd->sdIsymStart = visym;
	    sd->isdChild = isdNil;
	    sd->isdParent = isdParent;
	    sd->isdSibling = isdNil;
	    if (isdSibLast != isdNil)
	        vrgSd[isdSibLast].isdSibling = isdCur;
	    isdSibLast = isdCur;
	    if (vrgSd[isdParent].isdChild == isdNil)
	        vrgSd[isdParent].isdChild = isdCur;

	  /*
	   * Process enclosed scope
	   */
	    Scope (isdCur, visym);		/* this may invalidate sd */
	    vrgSd[isdCur].sdIsymEnd = visym;
	    break;

	case K_END:
	  /*
	   * Check if this is the end of the current scope
	   */
	    if (vsymCur->dend.beginscope.dnttp.index == isymRet)
		return;
	    break;

	}  /* switch */
    }  /* do forever */
}  /* Scope */

/************************************************************************
 * I N I T   S D
 *
 * This routine makes one pass over the entire DNTT to initialize the 
 * scope descriptor array.  
 */

void InitSd ()
{

#ifdef INSTR
    vfStatProc[365]++;
#endif

    visdInit = Max (6, (long) (visltMax * 0.10));
    visdIncr = Max (4, (long) (visltMax * 0.06));
    visdMax = visdInit;
    vrgSd = (pSDR) Malloc (visdInit * cbSDR, true);
    visdMac = isdNil + 1;			/* entry 0 is "NIL"	*/

    /* entry 1 is sentinel  */
    vrgSd[visdMac].isdChild  = isdNil;		/* no children scopes yet */
    vrgSd[visdMac].isdParent = isdNil;		/* no parent either	  */
    
    visym = -1;
    fnStdout = fileno (stdout);
    Scope (visdMac, 0);

    cBusy = 0;
    if (cBusyLine)			/* finish last line */
    {
	write (fnStdout, "\n", 1);	/* force out one char */
	cBusyLine = 0;
    }

    return;
}
#endif  /* HPSYMTAB */

#ifdef HPE


/************************************************
 *                                              *
 *  G E T   C O D E   A D R                     * 		
 *                                              *
 *  Assumes that iln may point to declarations  *
 *  or non-code for which compilers do not      *
 *  supply addresses.  Searches forward or back-*
 *  ward in the source file looking for a valid *
 *  code offset.                                *
 *                                              *
 ************************************************/

ADRT GetCodeAdr(ifd, iln)
    int ifd;	/* file to map */
    int iln;	/* line to map */

{
    ADRT adr;
    int  ilnMac = vrgFd[ifd].ilnMac;

#ifdef INSTR
    vfStatProc[366]++;
#endif

    if (ifd == vifdTemp)		/* temp files don't have code adrs */
	return adrNil;

    if (ilnMac == ilnNil)		/* no src file, forget bounds checks */
	return AdrFIfdLn(ifd, iln);

    if (iln > ilnMac) {

	iln = ilnMac - 1;
	for (; iln>0; iln--)
	    if ((adr=AdrFIfdLn(ifd, iln)) != adrNil)
		return adr;

        return adrNil;
    }

    if (iln < 0 ) iln = 0;
    
    for (; iln < ilnMac; iln++)
	if ((adr=AdrFIfdLn(ifd, iln)) != adrNil)
	    return adr;

    return adrNil;
}
#endif /* HPE */
