/* @(#) $Revision: 70.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * The routines here manage and list breakpoints, and insert or remove them
 * from the child process.
 */

#include <ctype.h>
#include "cdb.h"
#include "macdefs.h"


/***********************************************************************
 * BREAKPOINT INSTRUCTIONS:
 */

#define cbBp	2			/* size of a breakpoint */
#ifdef KDBKDB
short	vinsBp = 0x4e4f;		/* a "trap #15" instruction */
#else
short	vinsBp = 0x4e4d;		/* a "trap #13" instruction */
#endif

export int	vibpMac;	/* current next bp to allocate	*/

#define ibpMax 17
export BPR	vrgBp[ibpMax];		/* the bps */

/***********************************************************************
 * L I S T   B P
 *
 * Print two lines per breakpoint (one if there are no commands) if
 * there are any, or show one selected breakpoint.
 */

export void ListBp (ibpIn)
    int	ibpIn;		/* which one, or ibpNil */
{
    register int	ibp;		/* the current bp */

    if (vibpMac == ibpTemp + 1)
    {
	printf ("No breakpoints\n");		/* this is NOT an error */
	return;
    }
    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)
    {
	if ((ibpIn != ibpNil) AND (ibpIn != ibp))
	    continue;

	printf ("%2d: count: %d", ibp, vrgBp[ibp].count);
	if (vrgBp[ibp].count <= 0)
	    printf (" (temporary)");
	printf ("  ");

	/*  for debugging only:
	    printf ("addr: %d  ", vrgBp[ibp].adr);
	    printf ("inst: %d  ", vrgBp[ibp].inst);
	*/
	PrintPos (vrgBp[ibp].adr, fmtProc + fmtSave);

	if (vrgBp[ibp].sbBp[0] != chNull)		/* show commands */
	    printf ("    {%s}\n", vrgBp[ibp].sbBp);

    } /* for */

} /* ListBp */


/***********************************************************************
 * F   C L E A R   B P
 *
 * Clear the breakpoint at the given adr, if any.
 * Return true only if a breakpoint is cleared.
 *
 * Note: IbpFAdr() insures there is never more than one bp at an adr!
 */

export FLAGT FClearBp (adr, fTell)
    ADRT	adr;		/* adr of bp to clear   */
    FLAGT	fTell;		/* tell it was cleared? */
{
    register int	ibp;		/* the current bp */

    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)	/* scan the list */
    {
	if (vrgBp[ibp].adr == adr)			/* found it */
	{
	    if (fTell)
	    {
		printf ("Deleted:\n");
		ListBp (ibp);
	    }

	    Free (vrgBp[ibp].sbBp);			/* toss commands */

	    if (ibp < (--vibpMac))			/* not last bp */
		vrgBp[ibp] = vrgBp[vibpMac];

            kdb_purge();
	    return (true);
	}
    }
    return (false);

} /* FClearBp */


/***********************************************************************
 * C L E A R   A L L
 *
 * Clear all breakpoints or assertions, returning malloc'd memory.
 * Note that ibpTemp is not cleared, since it is handled whenever set.
 */

export void ClearAll (fClearBps)
    FLAGT	fClearBps;	/* clear bps not assertions? */
{
    register int	i;		/* current bp or assertion */

    if (fClearBps)				/* CLEAR BPS */
    {
	if (vibpMac == ibpTemp + 1)
	    printf ("No breakpoints\n");
	else
	{
	    for (i = ibpTemp + 1; i < vibpMac; i++)	/* return all memory */
		Free (vrgBp[i].sbBp);			/* (except ibpTemp)  */

	    vibpMac = ibpTemp + 1;			/* allow for the temp */
	    printf ("All breakpoints deleted\n");
	}
    }
    kdb_purge();
#ifdef KASSERT
    else					/* CLEAR ASSERTIONS */
    {
	if (viadMac == 0)
	    printf ("No assertions\n");
	else
	{
	    for (i = 0; i < viadMac; i++)	/* return all memory */
		Free (vrgAd[i].sbCheck);

	    viadMac = 0;
	    printf ("All assertions deleted\n");
	}
    }
#endif
} /* ClearAll */


/***********************************************************************
 * I B P   F   B R K
 *
 * If we are on a breakpoint, return it else return ibpNil
 */

export int IbpFBrk (pc)
    ADRT	pc;		/* current code location */
{
    register int	ibp;		/* current bp */

    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)	/* check the list */
    {
	if (vrgBp[ibp].adr == pc)			/* we are on one */
	    return (ibp);				/* return it */
    }
    return ibpNil;		/* not at bp */
}


/***********************************************************************
 * B R K   I N
 *
 * Install all the breakpoints (save real instructions and install bp
 * instructions ).
 */

export int BrkIn ()
{
    register int	ibp;		/* current bp */

    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)
    {
	GetBlock (vrgBp[ibp].adr, spaceText, (ADRT) & (vrgBp[ibp].inst), cbBp);
	PutBlock (vrgBp[ibp].adr, spaceText, (ADRT) & vinsBp,		 cbBp);
    }
    kdb_purge();

} /* BrkIn */


/***********************************************************************
 * I B P   F   B R K   O U T
 *
 * Restore original instructions for all active bps and see if we hit one.
 * Most systems stop past the bp so we may have to back up pc.
 * Clear any internal bps, and deal with temp or permanent ones hit.
 * If we somehow hit more than one (e.g. identical addresses), only the
 * last one is reportable; earlier ones are forgotten.
 *
 * Return:	ibpNil	     nothing was hit;
 *		ibpInternal  an internal was actually hit;
 *		ibpContinue  a bp was hit but not yet recognized;
 *		ibpTemp      a temp was recognized and "removed" (to ibpTemp);
 *		ibp	     other value if a permanent was recognized.
 *
 * Callers must hide ibpInternal or ibpContinue, or map them to ibpNil,
 * since some high-level places don't expect them.
 */

export int IbpFBrkOut (padr)
    ADRT	*padr;			/* current code location */
{
    int		ibpHit	= ibpNil;	/* bp we hit, if any	*/
    register int	ibp;			/* current bp		*/
    register pBPR	bp;			/* current bp record	*/
    ADRT	adr	= *padr;	/* fast copy of it	*/
    FLAGT	fBackUp = false;	/* have backed up yet?	*/

/*
 * SCAN THE LIST OF BPS:
 */

    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)
    {
	bp = vrgBp + ibp;				/* to access current */

/*
 * BACK UP PC:
 */
	if ((! fBackUp) AND (bp->adr + cbBp == adr))	/* we hit this bp  */
	{
	    *padr = (adr -= cbBp);
	    fBackUp = true;				/* don't do it again */
	}

/*
 * PUT BACK THE REAL INSTRUCTION FOR EVERY BP:
 */

	PutBlock (bp->adr, spaceText, (ADRT) & (bp->inst), cbBp);

/*
 * INTERNAL BP:
 */

	if (bp->count == 0)
	{
	    if (bp->adr == adr)			/* we hit this bp */
		ibpHit  =  ibpInternal;

	    FClearBp (vrgBp[ibp].adr, false);	/* clear it even if not hit   */
	    ibp--;				/* FClearBp() shuffles things */
	}

/*
 * TEMPORARY BP:
 */

	else if (bp->adr == adr)		/* we hit this bp */
	{
	    ibpHit = ibpContinue;		/* default == ignore it */

	    if (bp->count < 0)			/* a temporary bp */
	    {
		if ((bp->count += 1) == 0)	/* time to recognize it */
		{
		    char **psb = & vrgBp[ibpTemp].sbBp;	/* quick pointer */

		    if (*psb != sbNil)			/* has commands	   */
			Free (*psb);			/* free old memory */
		    vrgBp[ibpTemp] = *bp;	/* transfer to holding slot */

		    *psb = Malloc (strlen (bp->sbBp) + 1, "IbpFBrkOut");
		    strcpy (*psb, bp->sbBp);		/* save commands */

		    ibpHit = ibpTemp;
		    FClearBp (vrgBp[ibp].adr, false);	/* clear the real one */
		    ibp--;				/* undo "shuffle"     */
		}
	    }

/*
 * PERMANENT BP:
 */

	    else
	    {
		if ((bp->count -= 1) <= 0)	/* time to recognize it */
		{
		    bp->count = 1;		/* reset it */
		    ibpHit = ibp;
		}
	    }
	} /* if */
    } /* for */

    kdb_purge();
    return (ibpHit);

} /* IbpFBrkOut */


/***********************************************************************
 * I B P   F   A D R
 *
 * Save the address, count, and commands (if any) for a bp.  If one
 * already exists at an address, toss it and reuse the slot (except if
 * the new one is a single step bp only, in which case, do nothing).
 * Note that if fTell is false, the bp is assumed to be internal,
 * so no commands are saved and no report is made when it's hit.
 *
 * Note that sbCmd is the result of EatCmdList(), so it already has no
 * leading white space.  Also, sbBp always points to some malloc'd
 * memory, even if there are no commands, for simpler handling.
 */

export int IbpFAdr (adr, count, sbCmd, fTell)
    ADRT	adr;		/* where to set the bp	*/
    int		count;		/* its initial count	*/
    char	*sbCmd;		/* cmds that go with it */
    FLAGT	fTell;		/* list the new bp?	*/
{
    register int	ibp;	/* current bp	    */
    int		cb;		/* size of commands */

/*
 * CHECK ADDRESS:
 */

    if (adr == adrNil)
	UError ("Can't set breakpoint (invalid address)");

/*
 * SEE IF BP ALREADY EXISTS:
 */

    for (ibp = ibpTemp + 1; ibp < vibpMac; ibp++)	/* scan the list */
    {
	if (vrgBp[ibp].adr == adr)			/* already exists */
	{
	    if (count == 0)				/* new is single step */
		return (ibp);				/* keep old one	      */

	    printf ("Deleted:\n");			/* show it */
	    ListBp (ibp);
	    Free (vrgBp[ibp].sbBp);			/* toss commands  */
	    break;					/* reuse the slot */
	}
    }

/*
 * ADD NEW BP SLOT:
 */

    if (ibp == vibpMac)
    {
	if (vibpMac >= ibpMax)
	    UError ("Too many breakpoints");
	vibpMac++;
    }

/*
 * SAVE VALUES:
 */

    vrgBp[ibp].adr   = adr;
    vrgBp[ibp].count = count;
    vrgBp[ibp].fTell = fTell;

    cb = ((sbCmd == sbNil) OR (! fTell)) ? 0 : strlen (sbCmd);
    vrgBp[ibp].sbBp = Malloc (cb + 1, "IbpFAdr");	/* room for null */

    if (cb > 0)						/* must save cmds */
	strcpy (vrgBp[ibp].sbBp, sbCmd);

    vrgBp[ibp].sbBp[cb] = chNull;			/* trailing null */

/*
 * TELL NEW BP:
 */

    if (fTell)
    {
	printf ("Added:\n");
	ListBp (ibp);
    }

    kdb_purge();
    return (ibp);

} /* IbpFAdr */


/***********************************************************************
 * I B P   F   S E T
 *
 * Set a temporary or permanent bp in the array and return which if set.
 * Only special bps (except "ba") are handled here; simple ones are done
 * elsewhere.  Caller must set vipd for the default procedure.
 *
 * Note that sbCmd is the result of EatCmdList(), so it has no leading
 * white space.  Also, it can handle any stack depth, including zero, or
 * none at all (-1) for current procedure (which may differ from top of
 * stack).
 *
 * Calls itself recursively for trace bps, which are the only kind that
 * can have a proc name if not stack relative.
 */

export int IbpFSet (chStyle, cnt, sbCmd)
    char	chStyle;		/* kind of special bp	*/
    int		cnt;			/* stack depth or -1	*/
    char	*sbCmd;		/* commands, if any	*/
{
    FLAGT	fTemp = isupper (chStyle);  /* temp or perm?	*/
    FLAGT	fDoIt = false;		/* do set bp when done?	*/
    int		ibp   = ibpNil;		/* results, default nil */

    ADRT	pc = vpc;		/* reg values (default)	*/
    ADRT	fp = GetReg(ufp);

    int		ifdSave = vifd;		/* resettable values	*/
    int		ipdSave = vipd;
    int		ilnSave = viln;

    if (cnt >= 0)			/* want it stack relative: set ipd */
    {
	while ((cnt-- > 0) AND fp)		/* not off bottom yet	*/
	    NextFrame (&fp, &pc);		/* drop down one	*/

	if (fp == 0)
	    UError ("Stack isn't that deep");

    }

    switch (chStyle)			/* set bp or figure out where */
    {
	default:
	    UError ("Invalid breakpoint type \"%c\"", chStyle);


	case 'U':				/* uplevel breakpoint */
	case 'u':
	    pc	  = AdrFStackFix (pc);			/* will set at return */
	    fDoIt = true;
	    break;

    } /* switch */

    if (fDoIt)
	ibp = IbpFAdr (pc, (fTemp ? -1 : 1), sbCmd, true);	/* and tell */

    if (vifd != ifdSave)			/* reset the world */
	IfdFOpen (ifdSave);			/* ensure file is open */
    vipd = ipdSave;
    viln = ilnSave;

    return (ibp);				/* tell what we did */

} /* IbpFSet */
