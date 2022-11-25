/* @(#) $Revision: 70.2 $ */     
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * This large collection of routines is mostly for raw symbol and string
 * handling (e.g. as read from the object file).  There are also comparison
 * and initialization routines.
 */

#include "cdb.h"


/*
 * These are used for caching SYMBOL INFORMATION.
 *
 * For HPSYMTAB(II), "sym" refers to the DNTT, not the LST, and visymMax
 * is the number of DNTT blocks, not the number of symbols.
 */

export	pFDR	vrgFd;		/* file descriptors	*/
export	int	vifdMac;	/* total files		*/
export	int	vifd;		/* current file		*/

export	pPDR	vrgPd;		/* procedure descriptors */
export	int	vipdMac;	/* total procedures	 */
export	int	vipd;		/* current procedure	 */

export	pMDR	vrgMd;		/* module descriptors	*/
export	int	vimdMac;	/* total modules	*/

export	long	vcbSymFirst;	/* byte offset to first symbol in symfile */

export	pSYMR	vsymCur;	/* the "current" symbol */
export	long	visym;		/* current symbol index */
export	long	visymLim;	/* last	 sym in cache	*/
export	long	visymMax;	/* number of symbols	*/

export	long	visymGlobal;	/* isym of first global */
export	long	vipxdbheader;	/* offset in a.out of pxdb header */
export	long	vipdfdmd;	/* offset in a.out of quick-lookup tables */

SYMBLOCK *vsymCache;		/* the cdb symbol table pointer */

export	char	*vsbSymfile;	/* name of the symfile	*/
export	int	vfnSym;		/* symbol file number	*/

/*
 * These are used for caching STRING SPACE info (value table for HPSYMTABs).
 */

export	int	vcbSbFirst;	/* start of ss in file	*/
export	int	vcbSbCache;	/* size	 of the cache	*/
char		*vsbCache;	/* start of the cache	*/

/***********************************************************************
 * F   S B   C M P
 *
 * Compare two strings for equality and return true if they match,
 * including case [in]sensitivity.
 *
 * Case insensitivity is done a very crude way, by mapping the uppercase
 * ASCII quadrant (0100 - 0137) to the lowercase quad (0140 - 0177),
 * including non-letter chars.
 */

export FLAGT FSbCmp (sb1, sb2)
    register char    *sb1, *sb2;		/* strings to compare */
{
    if ((sb1 == sbNil) OR (sb2 == sbNil))	/* one is nil	    */
	return (sb1 == sb2);			/* true if both nil */

    while (*sb1 == *sb2)			/* match so far */
    {
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

export FLAGT FHdrCmp (sbHdr, sbCheck)
    register char	*sbHdr;		/* pattern to match */
    register char	*sbCheck;	/* string to check  */
{
    if ((sbHdr == sbNil) OR (sbCheck == sbNil))		/* one is nil	    */
	return (sbHdr == sbCheck);			/* true if both nil */

    while (*sbHdr == *sbCheck)    			/* match so far */
    {
	if (*sbHdr == chNull)				/* end of both */
	    return (true);				/* they match  */
	sbHdr++;
	sbCheck++;
    }
    return (*sbHdr == chNull);			/* match if substring */

} /* FHdrCmp */


/***********************************************************************
 * I N I T  C A C H E
 *
 * Read in all the DNTT's
 */

export void InitCache (cbSymCache)
    register long cbSymCache;
{
    vsymCache = (SYMBLOCK *)Malloc(cbSymCache,"InitCache");

    if (seekread(vfnSym, vcbSymFirst, vsymCache, cbSymCache, "InitCache") < 0)
	Panic (vsbErrSfailSD, "Read", "SetCache", cbSymCache);
    visymLim = visymMax;
}


/***********************************************************************
 * S E T   S Y M
 *
 * For HPSYMTAB(II) it might call itself recursively in order to advance to
 * the next DNTT block which is not an extension block.
 */

export void SetSym (isym)
    register long	isym;		/* symbol to set */
{
    if ((isym < 0) OR (isym >= visymMax))
    {	visym = isymNil;
	vsymCur = psymNil;
	return;
    }

    visym   = isym;
    vsymCur = (pSYMR) & (vsymCache[isym]); /* need cast for HPSYMTABs */

    if (vsymCur->dblock.extension)		/* oops, at extension block */
	SetSym (isym + 1);			/* recurse to go to next    */

} /* SetSym */


/***********************************************************************
 * S E T   N E X T
 *
 * Set to the symbol before the given one, in preparation for searching.
 *
 * It might call itself recursively in order to find the previous DNTT
 * block which is not an extension block (given that the SetSym()
 * routine advances visym forwards if we do hit one).
 */

export void SetNext (isym)
    register long	isym;		/* symbol to set */
{
    if ((isym < 0) OR (isym >= visymMax))
	return;

    if ((visym = isym - 1) >= isym0)	/* not before start of table */
    {
	SetSym (visym);			/* set to previous symbol */

	if (visym >= isym)		/* tried to set to extension block */
	    SetNext (--isym);		/* recurse to go back one more	   */

    }

} /* SetNext */


/***********************************************************************
 * F   N E X T	 S Y M
 *
 * Search symbols forward for the first one with a matching type and
 * optionally a matching name, or with the stop type.
 */

export FLAGT FNextSym (typHit1, typHit2, typHit3, typStop, sbHit, Cmp, typHit4, typHit5, typHit6)
    int		typHit1, typHit2, typHit3;	/* search types	     */
    int		typStop;			/* stop type	     */
    int		typHit4, typHit5, typHit6;	/* for HPSYMTAB(II) only */
    char	*sbHit;				/* name req'd if any */
    FLAGT	(* Cmp)();			/* name compare proc */
{
    register long isym = visym + 1;		/* current symbol */
    register int  i;				/* index of sym	  */
    register int  typ;				/* type of symbol */

/*
 * LOOK FOR NEXT SYMBOL:
 */

    while (isym < visymMax)
    {

	for (i = isym; (isym < visymLim); i++, isym++)
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
	    OR	  (typ == typHit6))
	    AND	 ((sbHit == sbNil) OR FNameCmp (sbHit, Cmp)) )
	    {
		visym = isym;			/* found a match */
		return (true);
	    }
	    else if (typ == typStop)		/* found a stop */
	    {
		printf("type was typStop\n");
		return (false);
	    }
	} /* for */
    } /* while */

    return (false);			/* off the end of the symbol table */

} /* FNextSym */




/***********************************************************************
 * I L N   F   I S Y M
 *
 * Map a symbol index to a source line number (current number or nearest).
 */

export int IlnFIsym (isym)
    long	isym;		/* symbol to map */
{
/*
 * Return the line number of the next normal line, if any, before the
 * next SLT of the same type this symbol references.  Must be called with
 * the index of a scope symbol.
 */
    SLTTYPE	sltStop;			/* where to stop */

    SetSlt (IsltFIsym (isym));			/* go to its SLT entry */
    sltStop = vsltCur->sspec.sltdesc;		/* remember start type */
    SetNextSlt (vislt + 1);			/* start with next one */

    if (FNextSlt (SLT_NORMAL, SLT_EXIT, SLT_NIL, sltStop))
	return (vsltCur->snorm.line);

    return (ilnNil);				/* didn't find one */


} /* IlnFIsym */



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

export FLAGT FNextVar (kind1, kind2, kind3, kind4, sbVar)
    KINDTYPE	kind1, kind2, kind3, kind4;	/* allowed kinds    */
    char	*sbVar;				/* pattern to match */
{
    register long	isymFirst = visym;	/* note our starting point */
    register long	isymNew;		/* for following param chain */
    register long	isymNest  = isymNil;	/* deeper scope start, if any */
    register long	isymBegin;		/* for lookup of scope start */
    register KINDTYPE	kind;			/* type of current symbol */

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
		isymNew = vsymCur->dfunc.firstparam.dnttp.index;
	    }
	    else if ((kind == K_FPARAM)				/* at param   */
	    AND (vsymCur->dfparam.nextparam.word != DNTTNIL))	/* more chain */
	    {
		isymNew = vsymCur->dfparam.nextparam.dnttp.index;
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

    while (FNextSym (kind1, kind2,	kind3, K_NIL, sbNil, nil,
		     kind4, K_FUNCTION,	K_END))
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
		if ((vsymCur->dend.endkind == K_FUNCTION) /* a start of scope  */
		|| (vsymCur->dend.endkind == K_MODULE)) {
		    SetSym (isymBegin);
		    break;				/* ran out of locals */
		}
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

    SetSym (isymFirst);				/* failed to find anything */
    return (false);

} /* FNextVar */


/***********************************************************************
 * A D R   F   I F D   L N
 *
 * Map a sourcefile descriptor and line number to a code address.
 */

export ADRT AdrFIfdLn (ifd, iln)
    int		ifd;	/* file to map */
    int		iln;	/* line to map */
{
/*
 * We have to search the whole vrgFd[] for every fd with the same filename.
 * For each, we can just scan its section of the SLT for the line number.
 * Return adrNil if (iln < min ln seen) or (iln > max ln seen) across each
 * section, e.g. iln must actually belong to one of the file sections.
 */

    char	*sbFile = vrgFd[ifd].sbFile;	/* filename to look for */
    register long ilnCur;			/* current line number	*/

    if (ifd == ifdNil) 
	return adrNil;

    for (ifd = 0; ifd < vifdMac; ifd++)
    {
	if (strcmp (sbFile, vrgFd[ifd].sbFile) == 0)	/* fd needs checking */
	{
	    SetSlt (IsltFIsym (vrgFd[ifd].isym));	/* set to SRCFILE */
	    if (vsltCur->sspec.line > iln)
		continue;				/* starts too high */

	    while (FNextSlt (SLT_NORMAL, SLT_EXIT, SLT_NIL, SLT_NIL))
	    {
		if (vsltCur->snorm.address > vrgFd[ifd].adrEnd)
			break;				/* try next fd	   */
		ilnCur = vsltCur->snorm.line;
		if (ilnCur == SLT_LN_PROLOGUE)		/* skip such lines */
			continue;

		if ((ilnCur >= iln) && 
                    (strcmp (sbFile, 
                             vrgFd[IfdFAdr(vsltCur->snorm.address)].sbFile
                            ) == 0)) {
		    return (vsltCur->snorm.address);
                }
	    } /* while */
	} /* if */
    } /* for */

    return (adrNil);					/* never found it */

} /* AdrFIfdLn */


/***********************************************************************
 * I F D   L N	 F   A D R
 *
 * Map a code address to the corresponding sourcefile and line number.
 */

export void IfdLnFAdr (adr, isym, pifd, piln, pslop)
    ADRT	adr;		/* address to map */
    long	isym;		/* start symbol	  */
    int		*pifd;		/* fd to return	  */
    int		*piln;		/* ln to return	  */
    int		*pslop;		/* offset to ret  */
{
    int		ipd;		/* current pd */

    register long	ilnLast = ilnNil;	/* previous line's values */
    register ADRT	adrLast = adrNil;

    if ((*pifd = IfdFAdr (adr)) == ifdNil)	/* unknown file */
	return;

    if (isym == 0)		/* not specified, start after nearest proc */
    {
	if ((ipd = IpdFAdr (adr)) == ipdNil)		/* unknown proc */
	    return;

	isym = vrgPd[ipd].isym;
    }

/*
 * If isym was not nil, it had better be a scoping DNTT entry.
 * The matching line is the last one whose address <= given address.
 * Existing semantics are sloppy.  I tried to do the right thing by
 * returning ilnNil if adr < min line address, else return line number
 * with slop >= 0.  Exception:	Return ilnNil if the line number is
 * SLT_LN_PROLOGUE.
 *
 * Assumes there are no normal SLT entries after the end of this
 * function but before the next one.
 */

    SetNextSlt (IsltFIsym (isym) + 1);	/* skip SLT_FUNCTION entry */

    while (FNextSlt (SLT_NORMAL, SLT_EXIT, SLT_NIL, SLT_FUNCTION)
    AND	   (vsltCur->snorm.address <= adr))
    {
	ilnLast = vsltCur->snorm.line;		/* save potential ln */
	adrLast = vsltCur->snorm.address;
    }

    if (ilnLast == SLT_LN_PROLOGUE)		/* hide special case */
	ilnLast = ilnNil;

    *piln  = ilnLast;
    *pslop = (ilnLast == ilnNil) ? (-1) : (adr - adrLast);

} /* IfdLnFAdr */



/***********************************************************************
 * I N I T   S S
 *
 * Initialize the string space cache.
 */

export void InitSs()
{
    if (vcbSbCache == 0)
    {	vsbCache = (char *) -1;
	return;
    }

    vsbCache = Malloc (vcbSbCache, "InitSs");

    if (seekread (vfnSym, vcbSbFirst, vsbCache, vcbSbCache, "InitSs") < 0)
	Panic (vsbErrSfailSD, "Seek", "InitSs", vcbSbFirst);
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
    char	*sb;		/* iss to read in or string ptr to fix */
{
    register unsigned long iss = (unsigned long) sb;	/* current iss */

    if (iss == -1)			/* nil pointer */
	return (sbNil);

/* This code is removed intentionally. It is kept in as a comment to show how
   bitHigh was being used. bitHigh will be changed to be zero and will become
   a nop where it is used. The assumption this kludge operated under became
   incorrect when the debugger was allowed to run in high ram.

    if (iss & bitHigh)
	return ((char *) (iss & ~bitHigh));

REMOVE THIS CODE */

    /* the assumption now is that if iss > 0x800000 then it must be a
       actual char pointer and not an index */

#ifndef CDBKDB
    if (iss > (unsigned long)0x800000)
#else /* CDBKDB */
    if (iss > sym_tab_size)
#endif /* CDBKDB */
	return ((char *) (iss));

    return (vsbCache+iss);

} /* SbInCore */


/***********************************************************************
 * I N I T   F D   P D
 *
 * Initializes the file, procedure and module descriptors (quick reference
 * arrays vrgFd, vrgPd and vrgMd) and related values.  Most of the work is
 * now done by pxdb (called by ld when the kernel is linked).
 */

export void InitFdPd()
{
    int	i;
    struct PXDB_header pxdbh;
    char	*sbTemp;	/* for name processing	*/
    long offset;

    printf("initializing symbolic debug information\n");
    visymLim = visymMax;

    if (!vipxdbheader) {
	memset(&pxdbh, 0, sizeof(struct PXDB_header));
    } else {

	/* Get pxdb header */
        seekread(vfnSym, vipxdbheader, &pxdbh, sizeof(struct PXDB_header), "pxdb");
    }

#ifdef CDBKDB
    if (display_headers) {
	printf("\n\nPXDB_header information:\n\n");

	printf("pxdb.pd_entries  (# procedures in look-up table) = %d\n", pxdbh.pd_entries);
	printf("pxdb.fd_entries  (# files in look-up table)      = %d\n", pxdbh.fd_entries);
	printf("pxdb.md_entries  (# modules in look-up table)    = %d\n", pxdbh.md_entries);
	printf("pxdb.pxdbed      (1 => has been preprocessed)	 = %d\n", pxdbh.pxdbed);
	printf("pxdb.bighdr      (1 => hdr contains 'time' word) = %d\n", pxdbh.bighdr);
	printf("pxdb.sa_header   (1 => SA version of pxdb)	 = %d\n", pxdbh.sa_header);
	printf("pxdb.spare       (spare)			 = %d\n", pxdbh.spare);
	printf("pxdb.globals	 (where GNTT begins in DNTT)	 = %d\n", pxdbh.globals);
	printf("pxdb.time	 (modify time before pxdbed)	 = %d\n", pxdbh.time);
	printf("pxdb.pg_entries	 (# labels in look-up table)	 = %d\n", pxdbh.pg_entries);
	printf("pxdb.spare2      (spare2)			 = %d\n", pxdbh.spare2);
	printf("pxdb.spare3      (spare3)			 = %d\n", pxdbh.spare3);
    }
#endif /* CDBKDB */

    /* Get procedure look-up table */
    vipdMac = pxdbh.pd_entries;		/* # of entries in vrgPd */
    vrgPd = (pPDR) Malloc (vipdMac * cbPDR, "InitFdPd #1");
    offset = vipdfdmd;
    seekread(vfnSym, offset, vrgPd, vipdMac *cbPDR, "look-up table");
    offset += vipdMac * cbPDR;

    /* Get file look-up table */
    vifdMac = pxdbh.fd_entries;		/* set # of entries in vrgFd */
    vrgFd = (pFDR) Malloc ((vifdMac + 2) * cbFDR, "InitFdPd #2");
    /* read(vfnSym, vrgFd, vifdMac *cbFDR); */
    seekread(vfnSym, offset, vrgFd, vifdMac *cbFDR, "file look-up table");
    offset += vifdMac * cbFDR;

    /* Get module look-up table */
    vimdMac = pxdbh.md_entries;		/* set # of entries in vrgFd */
    vrgMd = (pMDR) Malloc (vimdMac * cbMDR, "InitFdPd #3");
    /* read(vfnSym, vrgMd, vimdMac *cbMDR); */
    seekread(vfnSym, offset, vrgMd, vimdMac *cbMDR, "module look-up table");
    offset += vimdMac * cbMDR;

    visymGlobal = pxdbh.globals;	/* index into the DNTT where GNTT begins */

    for (i=0; i < vipdMac; i++)
    {
	sbTemp	= SbInCore (vrgPd[i].sbAlias);	/* save alias */
	vrgPd[i].sbAlias = Malloc (strlen (sbTemp) + 2, "InitFdPd #4");
	strcpy (vrgPd[i].sbAlias, sbTemp);

	sbTemp	= SbInCore (vrgPd[i].sbProc);	/* save name */
	vrgPd[i].sbProc	= Malloc (strlen (sbTemp) + 2, "InitFdPd #5");
	strcpy (vrgPd[i].sbProc, sbTemp);
    }

    for (i=0; i < vifdMac; i++)
    {
	sbTemp	= SbInCore (vrgFd[i].sbFile);
	vrgFd[i].sbFile = Malloc (strlen (sbTemp) + 2, "InitFdPd #6");
	strcpy (vrgFd[i].sbFile, sbTemp);
    }

    for (i=0; i < vimdMac; i++)
    {
	sbTemp	= SbInCore (vrgMd[i].sbAlias);	/* save alias */
	vrgMd[i].sbAlias = Malloc (strlen (sbTemp) + 2, "InitFdPd #7");
	strcpy (vrgMd[i].sbAlias, sbTemp);

	sbTemp	= SbInCore (vrgMd[i].sbMod);	/* save name */
	vrgMd[i].sbMod	= Malloc (strlen (sbTemp) + 2, "InitFdPd #8");
	strcpy (vrgMd[i].sbMod, sbTemp);
    }

    vifd = ifdNil;
    vipd = ipdNil;

} /* InitFdPd */



/***********************************************************************
 * N A M E   F	 C U R	 S Y M
 *
 * Return the name string for the current (named) symbol.
 * Does NOT know about aliases!
 */

export char * NameFCurSym ()
{
    long	iss;		/* where to find the name in VT */

    switch (vsymCur->dblock.kind)
    {

	default:	 Panic (vsbErrSinSDD, "Unnamed symbol type",
				"NameFCurSym", vsymCur->dblock.kind, visym);

	case K_SRCFILE:	 iss = vsymCur->dsfile.name ;	break;
	case K_MODULE:	 iss = vsymCur->dmodule.name;	break;
	case K_FUNCTION: iss = vsymCur->dfunc.name  ;	break;
	case K_ENTRY:	 iss = vsymCur->dentry.name ;	break;
	case K_IMPORT:	 iss = vsymCur->dimport.module;	break;
	case K_LABEL:	 iss = vsymCur->dlabel.name ;	break;
	case K_FPARAM:	 iss = vsymCur->dfparam.name;	break;
	case K_SVAR:	 iss = vsymCur->dsvar.name  ;	break;
	case K_DVAR:	 iss = vsymCur->ddvar.name  ;	break;
	case K_CONST:	 iss = vsymCur->dconst.name ;	break;
	case K_TYPEDEF:	 iss = vsymCur->dtype.name  ;	break;
	case K_TAGDEF:	 iss = vsymCur->dtag.name   ;	break;
	case K_MEMENUM:	 iss = vsymCur->dmember.name;	break;
	case K_FIELD:	 iss = vsymCur->dfield.name ;	break;
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

export FLAGT FNameCmp (sbHit, Cmp)
    char	*sbHit;		/* pattern to look for	  */
    FLAGT	(* Cmp)();	/* compare routine to use */
{
    long	kind;		/* kind of symbol */

    if ((*Cmp) (sbHit, NameFCurSym()))
	return (true);

    if ((kind = vsymCur->dblock.kind) == K_FUNCTION)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dfunc.alias)));

    if (kind == K_ENTRY)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dentry.alias)));

    if ((kind = vsymCur->dblock.kind) == K_MODULE)
	return ((*Cmp) (sbHit, SbInCore (vsymCur->dmodule.alias)));

    return (false);

} /* FNameCmp */
