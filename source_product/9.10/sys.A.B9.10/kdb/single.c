/* @(#) $Revision: 66.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines support single stepping and assertions.
 */

#include "cdb.h"
#include "macdefs.h"

export	pADR	vrgAd;			/* the list of assertions  */
export	int	viadMac;		/* current number of ass.  */
export	int	viadMax;		/* maximum allowable ass.  */

#ifdef KASSERT
export	ASE	vas;			/* external overall assertion state */
export	FLAGT	vfRunAssert = 1;	/* internal enable/disable flag	    */
local	char	*sbAssState = "Overall assertions state:  %s\n";
#endif

#ifdef KASSERT
/*
 * Macro to use to be sure vfRunAssert is reset at any return:
 * Users must set a LOCAL fRunSave first.
 */

#define CleanReturn(ret)	{ vfRunAssert = fRunSave; return (ret); }
#else
#define CleanReturn(ret)	{ return (ret); }
#endif


/***********************************************************************
 * A D R   F   S T A C K   F I X
 *
 * Given an address (pc) just beyond the return from a procedure call,
 * return the address past any stack fix-up instructions at that point.
 */

export ADRT AdrFStackFix (adr)
    ADRT	adr;		/* code pc */
{

#define TST_INST	0x4a5f
#define TSTL_INST	0x4a9f
#define CMPML_INST	0xbf8f
#define ADDW_INST	0xdefc
#define ADDQ_INST	0x500f
#define ADDN_INST	0x504f

    short	inst;

    GetBlock (adr, spaceText, &inst, 2);

    if ((inst == TST_INST)
    OR	(inst == TSTL_INST)
    OR	(inst == CMPML_INST)
    OR	((inst & 0xf13f) == ADDQ_INST))
	adr += 2;
    else if ((inst & 0xf1ff) == ADDN_INST)
	adr += 4;

    return (adr);

} /* AdrFStackFix */


/***********************************************************************
 * A D R   F   P R E A M B L E
 *
 * Given the first address (pc) in a procedure, return the address of the
 * first "real" instruction, i.e., after the csav0 or the brb whatever.
 */

local ADRT AdrFPreamble (adr)
    ADRT	adr;		/* code pc */
{
    int		ipd;				/* current procedure */
    ADRT	adrTemp;			/* of its first line */

    if ( ((ipd	   = IpdFAdr  (adr)) != ipdNil)
    AND	 ((adrTemp = AdrFIsym (vrgPd[ipd].isym, 0, tyNil)) != adrNil) )
    {
	return (adrTemp);			/* only if it's legitimate */
    }						/* else original value	   */

    return (adr);

} /* AdrFPreamble */


/***********************************************************************
 * F   A T   C A L L
 *
 * Tell whether the instruction at adr is a procedure call of any kind.
 * For FOCUS only, if at a call, also return the return address.
 */

local FLAGT FAtCall (adr)
    ADRT	adr;		/* code pc */
{
    unsigned short	inst = 0;	/* instruction of interest */

#define JSR_INST	0x4e80
#define BSR_INST	0x6100
#define cbInsMax	2

    GetBlock (adr, spaceText, (ADRT) &inst, cbInsMax);
    return (((inst & 0xff00) == BSR_INST) OR ((inst & 0xffc0) == JSR_INST));

} /* FAtCall */


#ifdef JUNK
/***********************************************************************
 * R E T   F   F P
 *
 * Given a frame pointer, return the return address for the procedure
 * above that frame.
 */

local ADRT RetFFp (fp)
    ADRT	fp;		/* frame pointer */
{
    ADRT	pc;		/* program adr */

    NextFrame (&fp, &pc);
    return (pc);

} /* RetFFp */
#endif /* JUNK */


/***********************************************************************
 * R E T   F   S P
 *
 * Given the stack pointer immediately after going through a procedure
 * call, return the return address from the procedure.
 */


local ADRT RetFSp (sp)
    ADRT	sp;		/* stack pointer */
{
    return (GetWord (sp));
} /* RetFSp */


/***********************************************************************
 * I B P   F   S I N G L E
 *
 * Single step the user process to the next SOURCE LINE (possibly
 * multiple machine instructions).  Start a new process first if needed.
 *
 * The big loop here looks at each machine instruction to handle one of
 * two basic cases:
 *
 *  1.	If we are at a proc call, step through.  If the new proc is not
 *	debuggable (or if we are big-stepping), immediately run till its
 *	return; otherwise just run past the intro code.
 *
 *  2.	If we are not at a proc call, keep looking for a source line, 
 *	stop if proc does not contain debug info.
 *
 */

export int IbpFSingle (fBigStep, fQuiet)
    FLAGT	fBigStep;		/* step over proc calls? */
    FLAGT	fQuiet;			/* tell location?	 */
{
#ifdef KASSERT
    FLAGT	fRunSave = vfRunAssert;	/* for CleanReturn()	*/
#endif
    FLAGT	fAtCall;		/* now at a proc call?	*/
    int		ibp;			/* bp hit, if any	*/
    int		ifd;			/* current file		*/
    int		iln;			/* current line		*/
    int		slop = -1;		/* byte offset from ln	*/
    ADRT	adr;			/* where to plant bp	*/

    if (vpid == pidNil)
	UError (vsbErrNoChild);
					
/*
 * LOOP UNTIL THE START OF A LINE:
 *
 * Starts of lines are addresses that translate exactly (slop == 0)
 * to a known line number, or any place a breakpoint is hit.
 *
 * Assertions are disabled while doing this and all returns from
 * this point down must use CleanReturn().
 */

#ifdef KASSERT
    vfRunAssert	= false;
#endif

    while (slop)
    {
	fAtCall = FAtCall (vpc);		/* note for below */

	if ((ibp = IbpFMachInst(false)) != ibpNil)	/* always step once */
	    CleanReturn (ibp);				/* must be at a line */
/*
 * AT A PROCEDURE CALL:
 */

	if (fAtCall)
	{


/*
 * STEP INTO DEBUGGABLE PROC by running up to its first line:
 */

	    if ((! fBigStep)			/* must step into proc	*/
	    AND (IpdFAdr(vpc) != ipdNil))	/* in a known proc	*/
	    {
		adr = AdrFPreamble (vpc);	/* skip intro code	*/
		IbpFAdr (adr, 0, sbNil, false);	/* set temp bp after it */
		ibp = IbpFRun (ptResume);	/* run past intro code	*/
		break;				/* get out of the loop	*/
	    }

/*
 * DON'T FOLLOW PROC CALL:
 */

	    adr = AdrFStackFix (RetFSp (GetReg(usp)));	/* skip cleanup after return */
	    IbpFAdr (adr, 0, sbNil, false);	/* set temp bp	after return */
	    ibp = IbpFRun (ptResume);		/* run past return	     */


	}					/* fall to later in loop */

/*
 * NOT AT A PROCEDURE CALL; see if at a bp or in known proc:
 */

	else
	{
	    if (IpdFAdr (vpc) == ipdNil)	/* no debug info for proc */
		break;				/* get out right away*/
	}

/*
 * SEE IF WE CAN QUIT:
 *
 * We can if we hit a bp, or we are at line.
 */

	if  (ibp != ibpNil)
	    CleanReturn (ibp);

	IfdLnFAdr (vpc, isym0, &ifd, &iln, &slop);	/* see where we are */

    } /* while (slop) */

/*
 * STEPPED INTO PROC OR FOUND A SOURCE LINE:
 */

    if (ibp == ibpNil)				/* no bp and child alive */
	PrintPos (vpc, fQuiet ? fmtNil : fmtFile + fmtProc);

    CleanReturn (ibp);

} /* IbpFSingle */


#ifdef KASSERT
/***********************************************************************
 * I N I T   A S S E R T
 *
 * Get memory for assertions and set the overall state.
 */

export void InitAssert()
{
    viadMac = 0;
    vrgAd   = (pADR) Malloc (viadMax * cbADR, "InitAssert");
    vas	    = asActive;

} /* InitAssert */


/***********************************************************************
 * L I S T   A S S E R T
 *
 * List all defined assertions.
 */

export void ListAssert (fShowAll)
    FLAGT	fShowAll;	/* not just last one? */
{
    register int	iad;	/* current assertion */

    if (viadMac == 0)
    {
	printf ("No assertions\n");		/* this is NOT an error */
	return;
    }

    printf (sbAssState, (vas == asActive) ? "ACTIVE" : "SUSPENDED");

    for (iad = (fShowAll ? 0 : viadMac-1); iad < viadMac; iad++)
    {
	printf ("%2d:  %-9s  {%s}\n", iad,
	    (vrgAd[iad].as == asActive) ? "Active" : "Suspended",
	     vrgAd[iad].sbCheck);
    }
} /* ListAssert */


/***********************************************************************
 * A D D   A S S E R T
 *
 * Add an assertion if there's room, save the commands, tell the user,
 * and set and tell the overall mode.  Note that it's added at the end,
 * so ListAssert (false) suffices to show it.
 */

export void AddAssert (sbCheck)
    char	*sbCheck;	/* assertion commands */
{
    register int	iad;	/* assertion to add */
    register int	cb;	/* size of commands */

    if (viadMac >= viadMax)
	UError ("Too many assertions");

    iad = viadMac++;
    cb	= strlen (sbCheck);

    vrgAd[iad].sbCheck = Malloc (cb + 1, "AddAssert");	/* room for null */

    strcpy (vrgAd[iad].sbCheck, sbCheck);
    vrgAd[iad].sbCheck[cb] = chNull;		/* trailing null */
    vrgAd[iad].as    = vas = asActive;		/* default state */

    ListAssert (false);				/* show last one only */

} /* AddAssert */


/***********************************************************************
 * M O D   A S S E R T
 *
 * Set the new state for an assertion (or remove it), and alter the
 * overall state if necessary.
 */

export void ModAssert (iad, as)
    register int	iad;	/* which to touch   */
    register ASE	as;	/* what to do to it */
{
    if ((iad < 0) OR (iad >= viadMac))
    {
	printf ("No assertion number %d\n", iad);
	ListAssert (true);			/* show all */
	return;
    }

    vrgAd[iad].as = as;

    printf ("Assertion %d %s\n", iad,	(as == asActive) ? "activated" :
					(as == asNil)	 ? "deleted"   :
							   "suspended");

    if (as == asActive)				/* activate it */
    {
	vas = asActive;				/* overall state too */
	printf (sbAssState, "ACTIVE");
	return;					/* and can quit now  */
    }
    if (as == asNil)				/* delete it */
    {
	Free (vrgAd[iad].sbCheck);
	vrgAd[iad] = vrgAd [viadMac - 1];	/* fold back on self */
	viadMac--;
    }
    if ((viadMac == 0) OR (vas == asSuspended))	/* none, or overall suspended */
	return;					/* safe to quit now	      */

/*
 * ASSERTION SUSPENDED OR REMOVED but some others left and overall is active:
 * See if any remaining is active and, if not, reset the overall state.
 */

    for (iad = 0; iad < viadMac; iad++)
	if (vrgAd[iad].as == asActive)		/* yes, one is */
	    return;

    vas = asSuspended;
    printf (sbAssState, "SUSPENDED");

} /* ModAssert */


/***********************************************************************
 * F   D O   A S S E R T
 *
 * Run and check assertions as we go.  Assertions are executed BEFORE
 * each line is executed.  We only stop if one encounters an "x" command,
 * in which case we show current status.
 *
 * This routine must never be called with pt == ptSingle!
 * All returns must use CleanReturn()!
 */

export void FDoAssert()
{
    register FLAGT	fRunSave = vfRunAssert;	/* for CleanReturn() */
    register int	iad;			/* current assertion */
    register ADRT	pcLast = adrNil;	/* previous address  */
    register int	ibp;

    vfRunAssert = false;		/* don't re-enter FDoAssert() */

    while (true)				/* till return in loop */
    {
	for (iad = 0; iad < viadMac; iad++)	/* check each assertion */
	{
	    if ((vrgAd[iad].as == asActive)		  /* this one active */
	    AND (FDoCommand (vrgAd[iad].sbCheck, false))) /* exit ("x") hit  */
	    {
		printf ("\nHit on assertion %d:  {%s}\n",
			iad, vrgAd[iad].sbCheck);

		if (pcLast != adrNil)
		{
		    printf   ("Last line executed was:\n");
		    PrintPos (pcLast, fmtFile + fmtProc + fmtSave);
		}
		printf	 ("Next line to execute is:\n");
		PrintPos (vpc, fmtFile + fmtProc);

		CleanReturn (true);		/* did hit assertion */
	    }
	} /* for */

	pcLast = vpc;				/* remember current line loc */

	ibp = IbpFMachInst (false);	/* single step */

	/* single step failed */
	if (ibp == ibpFail)
	    CleanReturn (false);		/* stepping into user space */

	/* hit bp */
	if (ibp != ibpNil)
	    CleanReturn (true);			/* did not hit assertion */

    } /* while (true) */

} /* FDoAssert */
#endif /* KASSERT */
