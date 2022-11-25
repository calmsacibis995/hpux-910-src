/* @(#) $Revision: 66.2 $ */      
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These routines manage child processes -- starting them,
 * waiting for them to stop, etc.
 */

#define stderr -1

#include <sys/param.h>

#define SYSTYPES 1			/* see basic.h */

#include "cdb.h"
#include "macdefs.h"

export	int	vibp;			/* last breakpoint hit, if any	*/

#ifdef JUNK
void Fixer();	/* need to reference as proc ptr but doesn't appear in ext.h */
#endif /* JUNK */


/***********************************************************************
 * I B P   F   W A I T
 *
 * Wait for a child process and figure out what happened to it.  This
 * routine handles terminal and signal settings, checks for bps, and
 * reports signals.  It should never be called outside the scope of
 * FDoCommand() because it saves bp commands for the latter to handle.
 *
 * It might see ibpInternal from IbpFBrkOut(), but if so, uses it and
 * maps it into ibpNil for its callers.  It does not "hide" ibpContinue
 * in this way.  
 */

local int IbpFWait (fDoBrk)
    FLAGT	fDoBrk;		/* OK to check breakpoints?	*/
{
    int		ibp;		/* breakpoint hit, if any	*/

	char chCmd;
	FLAGT	fBpRecog;			/* breakpoint recognized? */

	vibp = ibp = fDoBrk ? IbpFBrkOut (& vpc) : ibpNil;
	if (ibp == ibpInternal)		/* has served its purpose */
	    vibp = ibp =  ibpNil;	/* map it "out" right now */

	fBpRecog = (ibp != ibpContinue) AND (ibp != ibpNil); /* bp recognized */
	chCmd	 = (fBpRecog ? vrgBp[ibp].sbBp[0] : chNull); /* start of cmds */

	if (chCmd != chNull)			/* bp has commands */
	{
	    if ((vsbCmd != sbNil) AND (*vsbCmd != chNull))  /* had old ones */
		printf (vsbCmdsIgnored, vsbCmd);	    /* notify user  */

	    strcpy (vsbCmdBuf, ";");			    /* so parses OK */
	    vsbCmd = strcat (vsbCmdBuf, vrgBp[ibp].sbBp);   /* save new	    */
	}

	PrintPos(vpc, fmtNil);
	return ibp;

} /* IbpFWait */

/***********************************************************************
 * I B P   F   M A C H   I N S T
 *
 * Single step one machine instruction.
 */

export int IbpFMachInst(fBigStep)
{
    register int fAtCall;

    if (vpid == pidNil)
	UError (vsbErrNoChild);

    fAtCall = FAtCall(vpc);
    if (ptrace(ptSingle))
	return ibpFail;
    PrintPos(vpc, fmtNil);
    if (fBigStep && fAtCall)
    {
    	IbpFAdr(RetFSp(GetReg(usp)), 0, sbNil);
	IbpFRun (ptResume);
    }
    return IbpFBrk(vpc);
}

/***********************************************************************
 * I B P   F   R U N
 *
 * Run or step an existing child process, either via assertions or
 * normally.  Does not expect to see ibpInternal from IbpFWait(). 
 * Only call this when pt != ptSingle.
 */

export int IbpFRun (pt)
    int		pt;			/* ptrace() status to use    */
{
    int		ibp;			/* current bp, if any	     */

    if (vpid == pidNil)
	UError (vsbErrNoChild);

#ifdef KASSERT
/*
 * DO ASSERTIONS if that's required:
 *
 * Note that single stepping takes priority.
 */

    if ((vfRunAssert)				/* internally enabled	*/
    AND (vas == asActive)			/* user enabled		*/
    AND (viadMac > 0))				/* there are some	*/
    {
	if (FDoAssert())			/* assert or bp hit */
	    return (ibpNil);
	
		/* if stepping into user space, continue */
	printf("Continuing into user space, ignoring assertions\n");
    }
#endif

/*
 * PUT IN BREAKPOINTS, but step over current one, if any:
 */

    do				/* allow for bps that continue */
    {
	if (IbpFBrk (vpc) != ibpNil)		/* now at a bp; step over it */
	    ptrace (ptSingle);
        BrkIn (adrNil);			/* put in bps again */
	ptrace (pt);

	/* wait for results */
	ibp = IbpFWait (true);

    } while (ibp == ibpContinue);	/* bp encountered but not recognized */

    return (ibp);

} /* IbpFRun */


