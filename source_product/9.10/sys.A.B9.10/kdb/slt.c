/* @(#) $Revision: 70.2 $ */     
/*
 * Copyright Hewlett Packard Company, 1983.  This module is part of
 * the CDB symbolic debugger.  It was written from scratch by HP.
 */

/*
 * These routines support accesses of the SLT (source line table) in the
 * object file.	 This file should only be included for HPSYMTAB compilations.
 */

#include "cdb.h"

/*
 * These are used for caching SLT entries:
 */

local	pSLTR	vsltCache;	/* the cache itself	*/

export	long	vcbSltFirst;		/* byte offset to first SLT ent */
export	long	visltMax;			/* max in cache		*/

export	long	vislt;				/* last SLT used	*/
export	pSLTR	vsltCur;			/* the current entry	*/

/***********************************************************************
 * I N I T   S L T
 * Initialize the SLT by reading the entire table in--used only by KDB
 */

export void InitSlt(cbSltCache)
{

    if (cbSltCache == 0)
    {	vsltCache = psltNil;
	return;
    }

    vsltCache = (pSLTR)Malloc(cbSltCache,"InitSlt");

    seekread(vfnSym, vcbSltFirst, vsltCache, cbSltCache, "SetSltCache");
}



/***********************************************************************
 * S E T   S L T
 *
 * Set global SLT values to the given SLT entry.
 */

export void SetSlt (islt)
    long	islt;		/* index to set */
{

    vislt   = islt;
    vsltCur = & (vsltCache [islt]);

} /* SetSlt */


/***********************************************************************
 * S E T   N E X T   S L T
 *
 * Set globals to the given SLT entry preparatory to doing some searching.
 * Actually sets us to the previous entry, which is then skipped.
 */

export void SetNextSlt (islt)
    long	islt;		/* index to set */
{

    if ((vislt = islt - 1) >= islt0)
	SetSlt (vislt);

} /* SetNextSlt */


/***********************************************************************
 * F   N E X T	 S L T
 *
 * Look forward in the SLT, starting at the NEXT entry, for one of the
 * given types, but stop at typStop.
 */

export FLAGT FNextSlt (typHit1, typHit2, typHit3, typStop)
    long	typHit1, typHit2, typHit3;	/* search types */
    long	typStop;			/* stop type	*/
{
    long	islt = vislt + 1;		/* current loc	 */
    register int	i;			/* current index */
    register int	typ;			/* current type	 */

    for (i = islt; (islt < visltMax); i++, islt++)
    {
	vsltCur = & (vsltCache[i]);
	typ = vsltCur->sspec.sltdesc;

	if ((typ == typHit1) OR (typ == typHit2) OR (typ == typHit3))
	{
	    vislt = islt;
	    return (true);
	}
	else if (typ == typStop)
	{
	    return (false);
	}
    }

    return (false);			/* off the end of the SLT */

} /* FNextSlt */


/***********************************************************************
 * A D R   F   I S L T
 *
 * Find the next normal SLT entry, beginning with islt, and return its
 * code address.  Don't go past the end of the current function.
 */

export ADRT AdrFIslt (islt)
    long	islt;		/* index to get adr of */
{
    SetNextSlt (islt);

    if (FNextSlt (SLT_NORMAL, SLT_NIL, SLT_NIL, SLT_FUNCTION))
	return (vsltCur->snorm.address);
    else
	return (adrNil);			/* can't find an address */

} /* AdrFIslt */


/***********************************************************************
 * I S L T   F	 I S Y M
 *
 * Given the index of a scope symbol, return the index of its SLT entry.
 */

export long IsltFIsym (isym)
    long	isym;		/* symbol to do */
{
    SetSym (isym);

    switch (vsymCur->dblock.kind)
    {

	default: Panic (vsbErrSinSDD, "Wrong symbol type", "IsltFIsym",
				      vsymCur->dblock.kind, isym);

	case K_SRCFILE:		return (vsymCur->dsfile.address );
	case K_MODULE:		return (vsymCur->dmodule.address);
	case K_FUNCTION:	return (vsymCur->dfunc.address	);
	case K_ENTRY:		return (vsymCur->dentry.address );
	case K_BEGIN:		return (vsymCur->dbegin.address );
	case K_END:		return (vsymCur->dend.address	);
	case K_LABEL:		return (vsymCur->dlabel.address );
    }
} /* IsltFIsym */


/***********************************************************************
 * A D R   F   I S Y M
 *
 * Given the index of a scope symbol, return whatever address it has.
 * In the case of data addresses, take full account of indirection,
 * register types, and offsets.
 *
 * If there is no child process and a storage-type object has an indirect
 * address, returns a special adrUnknown, different than adrNil.  This
 * tells the caller that the global was found, but the address is of course
 * invalid.  However, anyone trying to use it should run into "no child
 * process" before "bad access".
 */

export ADRT AdrFIsym (isym, fp, rgTy)
    long	isym;			/* symbol to get data from	*/
    ADRT	fp;			/* frame ptr for fp-rel symbols */
    pTYR	rgTy;			/* to possibly modify here	*/
{
    ADRT	adr;				/* working value	*/
    register FLAGT	fIndir	= false;	/* adr is indirect	*/
    register long	offset	= 0;		/* after indirection	*/
    register FLAGT	fReg	= false;	/* actually register	*/
    register FLAGT	fUseFp	= false;	/* adr is fp-relative	*/

    SetSym (isym);

    switch (vsymCur->dblock.kind)
    {

	default: Panic (vsbErrSinSDD, "Wrong symbol type", "AdrFIsym",
				       vsymCur->dblock.kind, isym);

/*
 * LOCATION IS THE ADDRESS OF THE NEXT NORMAL LINE after the matching SLT entry:
 * Note that we skip the matching SLT entry itself, to avoid confusion.
 */

	case K_SRCFILE:
	case K_MODULE:
	case K_FUNCTION:
	case K_ENTRY:
	case K_BEGIN:
	case K_END:
	case K_LABEL:	return (AdrFIslt (IsltFIsym (isym) + 1));

/*
 * LOCATION IS IN SYMBOL ITSELF:
 */

	case K_FPARAM:	adr	= vsymCur->dfparam.location;
			fIndir	= vsymCur->dfparam.indirect;
			fReg	= vsymCur->dfparam.regparam;
			fUseFp	= ! fReg;		/* one or the other */
			break;

	case K_SVAR:	adr	= vsymCur->dsvar.location;
			fIndir	= vsymCur->dsvar.indirect;
			offset	= vsymCur->dsvar.offset;
			break;

	case K_DVAR:	adr	= vsymCur->ddvar.location;
			fIndir	= vsymCur->ddvar.indirect;
			fReg	= vsymCur->ddvar.regvar;
			fUseFp	= ! fReg;		/* one or the other */
			break;

	case K_CONST:	adr	= vsymCur->dconst.location;
			fIndir	= vsymCur->dconst.indirect;
			offset	= vsymCur->dconst.offset;

			if (vsymCur->dconst.locdesc == LOC_VT)
			    Panic (vsbErrSinSD, "Can't handle constant in VT",
						"AdrFIsym", visym);
    } /* switch */

/*
 * HAVE A DATA ADDRESS that may need more work, per the values set above:
 */

    if (fUseFp)
    {
	if (fp == 0)				/* simple, inaccurate test */
	    Panic (vsbErrSinSD, "Illegal fp == 0", "AdrFIsym", isym);
	adr += fp;				/* it was fp-relative */
    }
    if (fReg)
    {
	adr = AdrFReg (adr, fp, rgTy);		/* finds register values */
    }
    if (fIndir)
    {
	ADRT	adrT = adr;

	if (vpid == pidNil)			/* no child process */
	    return (adrUnknown);		/* prevent bombout  */

	GetBlock (adrT, spaceData, &adr, 4);	/* replace adr with contents */


    }
    return (adr + offset);

} /* AdrFIsym */

#ifdef JUNK
/***********************************************************************
 * F D	 B O U N D S   F   I S L T
 *
 * Given the index of an SLT_SRCFILE entry, search its range of SLT entries
 * (until the next SLT_SRCFILE or the end of the SLT) and save the address
 * bounds for the file descriptor.  The bounds are the min and max of a number
 * of addresses, including:
 *
 *	- Line start addresses of all SLT_NORMALs in the range.
 *	- Entry addresses (in DNTT) for all SLT_FUNCTIONs in the range.
 *	- Next	addresses (in DNTT), less one, for all SLT_ENDs in the range
 *	  which point to DNTT_ENDs which in turn point to DNTT_FUNCTIONs.
 *	- If the last SLT in the range is not one of these FUNCTION SLT_ENDs
 *	  nor a MODULE SLT_END, also include the address, less one, of the
 *	  very next SLT_NORMAL or FUNCTION SLT_END (as a "cap").  BUT, be
 *	  careful not to infinite loop if there is none.
 *
 * The reason for doing fExtra loops is to find the highest address in the
 * file, in the case where the user does an include of code before the end
 * of a procedure.
 */

export void FdBoundsFIslt (fd, islt)
    pFDR	fd;				/* where to save values	   */
    long	islt;				/* SLT_SRCFILE to start at */
{
    register ADRT	adrMin	 = 0x7fffffff;	/* more than any address   */
    register ADRT	adrMax	 = 0;		/* less than any address   */
    register ADRT	adrCur	 = 0;		/* current address	   */
    FLAGT	fExtra	 = false;		/* cont past next SRCFILE  */
    FLAGT	fQuit	 = false;		/* exit search loop	   */
    SLTTYPE	sltOpt	 = SLT_FUNCTION;	/* optional search type	   */
    SLTTYPE	sltStop	 = SLT_SRCFILE;		/* optional stop type	   */
    SLTTYPE	sltPrev	 = SLT_NORMAL;		/* type of previous entry  */
    long	isymSave = visym;		/* to reset DNTT when done */

    SetNextSlt (islt + 1);			/* skip SLT_SRCFILE entry */

    while (! fQuit)				/* until all conditions met */
    {
	if (FNextSlt (SLT_NORMAL, sltOpt, SLT_END, sltStop))	/* got one */
	{
	    /*
	     * Note that if fExtra, we are on the extra pass, looking only
	     * for SLT_NORMAL, SLT_END, or end of SLT, but NOT SLT_FUNCTION
	     * or SLT_SRCFILE (latter as a stop condition).
	     */

	    sltPrev = vsltCur->snorm.sltdesc;		/* note its type */

	    switch (sltPrev)
	    {

		case SLT_NORMAL:			/* use adr or adr-1 */
		    adrCur = vsltCur->snorm.address - (fExtra == true);
		    fQuit  = fExtra;			/* quit if on extra */
		    break;

		case SLT_FUNCTION:			/* never if fExtra */
		    /* set to FUNC DNTT entry */
		    SetSym (vsltCur->sspec.backptr.dnttp.index);
		    adrCur = vsymCur->dfunc.lowaddr;	/* use lowest adr */
		    break;

		case SLT_END:
		    /* set to END DNTT entry */
		    SetSym (vsltCur->sspec.backptr.dnttp.index);
		    /* set to FUNC DNTT entry (?) */
		    SetSym (vsymCur->dend.beginscope.dnttp.index);

		    if (vsymCur->dblock.kind == K_FUNCTION)	/* right type */
		    {
			adrCur = vsymCur->dfunc.hiaddr;
			fQuit  = fExtra;		/* quit if on extra */
			break;
		    }
		    else {
			if (vsymCur->dblock.kind != K_MODULE)	/* not module */
			    sltPrev = SLT_NIL;			/* wrong type */

			continue;			   /* no valid adrCur */
		    }

	    } /* switch */

	    adrMin = Min ((long) adrMin, (long) adrCur);
	    adrMax = Max ((long) adrMax, (long) adrCur);

	}
	else if (fExtra OR (sltPrev == SLT_END)) /* search failed; can quit */
	{
	    break;
	}
	else					/* search failed; can't quit */
	{
	    fExtra = true;			/* need extra search	 */
	    sltOpt = sltStop = SLT_NIL;		/* change the conditions */
	}

    } /* while */

    fd -> adrStart = adrMin;			/* save results */
    fd -> adrEnd   = adrMax;
    SetSym (isymSave);				/* reset symbols */

} /* FdBoundsFIslt */
#endif /* JUNK */
