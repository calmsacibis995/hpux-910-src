/* @(#) $Revision: 66.2 $ */    
/*
 * Copyright Third Eye Software, 1983.	This module is part of the CDB
 * symbolic debugger.  It is available to Hewlett-Packard Company under
 * an explicit source and binary license agreement.  DO NOT COPY IT
 * WITHOUT PERMISSION FROM AN APPROPRIATE HP SOURCE ADMINISTRATOR.
 */

/*
 * These are most (but not all) of the routines that list various groups
 * of things listable by "l" commands.
 */

#include "macdefs.h"
#include "cdb.h"
#include "Kdb.h"


/***********************************************************************
 * L I S T   F I L E S
 *
 * Show the list of source files from the symbol table.
 * There are always files; no need to check for none as a special case.
 */

local void ListFiles (sbFile)
    char	*sbFile;		/* pattern, if any  */
{
    register int	ifd;		/* current file	    */
    register pFDR	fd;		/* pointer to it    */

    for (ifd = 0; ifd < vifdMac; ifd++)
    {
	fd = vrgFd + ifd;

	if ((sbFile != sbNil) AND (! FHdrCmp (sbFile, fd->sbFile)))
	    continue;				/* exclude this one */

	printf ("%2d:  %-30s  %#lx", ifd, fd->sbFile, lengthen (fd->adrStart));
	printf (" to %#lx\n", lengthen (fd->adrEnd));

    } /* for */

} /* ListFiles */


/***********************************************************************
 * L I S T   P R O C S
 *
 * Show the list of debuggable procedures from the symbol table.
 * There are always procs; no need to check for none as a special case.
 */

local void ListProcs (sbProc)
    char	*sbProc;		/* pattern, if any   */
{
    register int	ipd;		/* current procedure */
    register pPDR	pd;		/* pointer to it     */
    int   startifd,  endifd;
    int   startline, endline;
    int   startslop, endslop;
    char *srcfile;
    pFDR  fdp;

    for (ipd = 0; ipd < vipdMac; ipd++)
    {
	pd = vrgPd + ipd;

	if ((sbProc != sbNil) AND (! FHdrCmp (sbProc, pd->sbProc)))
	    continue;			/* exclude this one */

	printf ("%2d:  %-15s  %#lx", ipd, pd->sbProc, lengthen (pd->adrStart));
	printf (" to %#lx", lengthen (pd->adrEnd));

	IfdLnFAdr (pd->adrBp, 0, &startifd, &startline, &startslop);
	if (startifd == ifdNil) {
	    printf ("  [ no source file ]\n");
	} else {
	    fdp = vrgFd + startifd;
	    IfdLnFAdr (pd->adrEnd, 0, &endifd, &endline, &endslop);
	    if ((srcfile = strrchr (fdp->sbFile, '/')) == (char *)0)
		srcfile = fdp->sbFile;
	    else
		++srcfile;
	    printf ("  [ %s: %d - %d ]\n", srcfile, startline, endline);
	}

	if (pd->sbAlias[0] != chNull)		/* has an alias name */
	    printf ("%2d:  %s\n", ipd, pd->sbAlias);

    } /* for */

} /* ListProcs */


/***********************************************************************
 * L I S T   G L O B A L S
 *
 * Show all global variables from the symbol table, with addresses or
 * values.  Does NOT let DispVal() use alternate form, in case a pointer
 * is invalid (don't want to bomb out partway through).
 */

local void ListGlobals (sbGlobal)
    char	*sbGlobal;		/* pattern, if any	*/
{
    char	*sbName;		/* full variable name	*/
    long	adrLong;		/* location of variable */
    TYR		rgTy[cTyMax];		/* type of variable	*/
    FLAGT	fPrint = false;		/* printed any?		*/
    int		imd;			/* current module	*/
    pMDR	pmd;			/* pointer to module	*/
    FLAGT	fPrintMod = false;	/* printed any Mod Glbs */
    FLAGT	fPrintGlb = false;	/* printed any Globals  */
    FLAGT	fTemp;			/* temp flag		*/

    if (vpid == pidNil)
    {
	printf ("No child process\n");
	return;
    }

/*
 * LOOK UP MODULE GLOBALS (external statics for C)
 */
    imd = ImdFIpd(vipd);
    if (imd != imdNil) {
	pmd = &vrgMd[imd];
	if (pmd->vars_in_front || pmd->vars_in_gaps) {
	    SetNext(pmd->isym);
	    fTemp = (!pmd->vars_in_gaps);
	    while (fTemp ?
		   FNextSym(K_SVAR, K_CONST, K_NIL, K_FUNCTION, sbNil, FSbCmp,
			    K_NIL, K_NIL, K_NIL ) :
		   FNextVar(K_SVAR, K_CONST, K_NIL, K_NIL, sbNil)) {
		sbName  = SbInCore(SbSafe(NameFCurSym()));
		if ((sbGlobal != sbNil) AND (! FHdrCmp(sbGlobal,sbName)))
		    continue;
		TyFGlobal (rgTy, sbName);
		adrLong = AdrFIsym (visym, 0, tyNil);
		if (!fPrint)
		    printf ("External Statics:\n");
		DispVal (adrLong, rgTy, modeNil, true, true, false);
		printf ("\n");
		fPrintMod = true;
		fPrint = true;
	    }
	}
    }

    SetNext (visymGlobal);

/*
 * LOOK UP NEXT GLOBAL:
 */

    while (FNextVar (K_SVAR, K_CONST, K_NIL, K_NIL, sbNil))
    {
	sbName  = NameFCurSym();

	if ((sbGlobal != sbNil) AND (! FHdrCmp (sbGlobal, sbName)))
	    continue;			/* exclude this one */

	adrLong = AdrFIsym (visym, 0, tyNil);	/* send fp == 0 for global */

/*
 * PRINT NAME AND VALUE:
 */

	TyFGlobal (rgTy);

	if (fPrintMod && !fPrintGlb)
		printf("Globals:\n");

	DispVal (adrLong, rgTy, modeNil, true, true, false);
	printf  ("\n");
	fPrint = true;
	fPrintGlb = true;

    } /* while */

    if (! fPrint)
	printf ("No %sglobals\n", (sbGlobal == sbNil) ? "" : "matching ");

} /* ListGlobals */

/***********************************************************************
 * L I S T   T Y P E S
 *
 * Show all global types from the symbol table, with addresses or
 * values.  Does NOT let DispVal() use alternate form, in case a pointer
 * is invalid (don't want to bomb out partway through).
 */

local void ListTypes (sbGlobal)
    char	*sbGlobal;		/* pattern, if any	*/
{
    char	*sbName;		/* full variable name	*/
    FLAGT	fPrint = false;		/* printed any?		*/

    SetNext (visymGlobal);

    if (vpid == pidNil)
    {
	printf ("No child process\n");
	return;
    }

    while (FNextSym(K_TYPEDEF, K_TAGDEF, K_NIL, K_NIL, sbNil, sbNil, 
		K_NIL, K_NIL, K_NIL))
    {
	sbName  = NameFCurSym();
	if ((sbGlobal != sbNil) AND (! FHdrCmp (sbGlobal, sbName)))
	    continue;			/* exclude this one */

	printf  ("%s\n",sbName);
	fPrint = true;

    } /* while */

    if (! fPrint)
	printf ("No %stypes\n", (sbGlobal == sbNil) ? "" : "matching ");

}

/***********************************************************************
 * L I S T   L O C A L S
 *
 * Show all params and local variables for the given or current
 * procedure, with values if available.  Does NOT let DispVal() use
 * alternate form, in case a pointer is invalid (don't want to bomb
 * out partway through).
 */

local void ListLocals (sbProc, count)
    char	*sbProc;		/* name of procedure	 */
    int		count;			/* how deep on stack	 */
{
    int		ipd;			/* procedure of interest */
    ADRT	fp;			/* frame ptr		 */
    char	*sbName;		/* name of variable	 */
    ADRT	adrLong;		/* location of variable	 */
    TYR		rgTy[cTyMax];		/* type of variable	 */
    FLAGT	fPrint = false;		/* printed any?		 */

    if (sbProc != sbNil)				/* given a name */
    {
	if ((ipd = IpdFName (sbProc)) == ipdNil)	/* look it up	*/
	    UError ("No such procedure \"%s\"", sbProc);
    }
    else						/* no name given */
    {
	if ((ipd = vipd) == ipdNil)			/* use current */
	    UError ("No current procedure");
    }
    FpFIpd (&fp, ipd, count);			/* look up proc */

    if (fp == 0)
	printf ("Procedure \"%s\" not active; here are the names:\n",
	    vrgPd[ipd].sbProc);

/*
 * LOOK UP NEXT LOCAL:
 */

    SetNext (vrgPd[ipd].isym + 1);

/*
 * Note that "local" is based on scope, not storage class:
 */
    while (FNextVar (K_FPARAM, K_SVAR, K_DVAR, K_CONST, sbNil))
    {
	sbName = NameFCurSym();

/*
 * PRINT NAME AND VALUE:
 *
 * Until (unless) AdrFLocal() has a way to know to only look at the current
 * visym, and not search again, we need the save-and-restore to prevent
 * looping in the case of two or more locals with the same name.
 */
	if (! fp)				/* no data available */
	    printf ("%s", sbName);
	else
	{
	    long isymSave = visym;
	    adrLong = AdrFLocal (ipd, count, sbName, rgTy);
	    SetSym (isymSave);
	    DispVal (adrLong, rgTy, modeNil, true, true, false);
	}
	printf ("\n");
	fPrint = true;

    } /* while */

    if (! fPrint)
	printf ("No locals\n");

} /* ListLocals */


/***********************************************************************
 * L I S T   S O M E T H I N G
 *
 * Figure out what to list (by reading the command line) and call a
 * routine to do the listing.  Either the current token contains the
 * option letter or it's safe to eat one more token because it's either
 * a separator or something we'll use (or complain about).  Look ahead
 * for an optional pattern for some forms but don't actually eat it
 * unless it exists and is used.
 */

export void ListSomething()
{
    char	chOpt;		/* option letter    */
    char	*sbPatt;	/* optional pattern */
    FLAGT	fOneTok = true;	/* all one token?   */
    FLAGT	fDidIt  = true;	/* did an action?   */
    long	depth	= -1;	/* use any depth    */
    char 	sbProc[80];	/* save proc name   */

    if (vcbTok > 2)			/* too many letters   */
	chOpt = '?';
    else if (vcbTok > 1)		/* single token */
	chOpt = vsbTok[1];
    else				/* separate tokens? */
    {
	TkNext();			/* set to option */
	chOpt	= (vcbTok == 1) ? vsbTok[0] : '?';
	fOneTok	= false;		/* was a naked "l" command */
    }

    sbPatt = (TkPeek() == tkStr) ? vsbPeek : sbNil;	/* possible pattern */
    vipd   = IpdFAdr (AdrFIfdLn (vifd, viln));		/* set current pos  */

/*
 * OPTION LETTER SPECIFIED (might be special case)?
 */

    switch (chOpt)
    {
	default:   fDidIt = false;			break;

#ifdef KASSERT
	case 'a':  ListAssert (true); sbPatt = sbNil;	break;	/* list all */
#endif
	case 'b':  ListBp (ibpNil);   sbPatt = sbNil;	break;	/* list all */
	case 'f':  ListFiles	     (sbPatt);		break;
	case 'g':  ListGlobals	     (sbPatt);		break;
	case 'l':  ListLabels	     (sbPatt);		break;
	case 'p':  ListProcs	     (sbPatt);		break;
	case 'r':  ListRegs	     ();		break;
	case 's':  ListSpecial	     (sbPatt);		break;
	case 't':  ListTypes	     (sbPatt);		break;
	case 'x':  ListXtra	     ();		break;
    }

    if (fDidIt)					/* did something */
    {
	if (sbPatt != sbNil)			/* used a pattern     */
	    TkNext();				/* set to it (eat it) */
	return;
    }

/*
 * SECOND TOKEN MIGHT BE PROC NAME:
 *
 * vtk is set to the first token if fOneTok, else to the second token.  Be
 * careful to leave vtk to the last token used, or a separator, if we don't
 * error out.
 */

    if (FAtSep (vtk))				/* end of command */
    {
	ListLocals (sbNil, -1);			/* current proc, any depth */
    }
    else if (fOneTok OR (vtk != tkStr))		/* bad form of cmd */
    {
	UError ("Unrecognized \"l\" command");
    }
    else					/* treat token as proc name */
    {
	strcpy(sbProc,vsbTok);
	if (TkPeek() == tkDot)			/* "proc.depth" */
	{
	    TkNext();				/* set to "."	  */
	    TkNext();				/* set to unknown */

	    if (! FValFTok (& depth))
		UError ("\".\" must be followed by a depth");
	}
	ListLocals(sbProc, depth);
    }

} /* ListSomething */

/***********************************************************************
 * L I S T  X T R A
 *
 * List the extra values that kdb stores and/or replaces.
 */

ListXtra()
{
#ifndef CDBKDB
	printf("sfc  %#x    dfc  %#x\n",kdb_fregs[0],kdb_fregs[1]);
	printf("usp  %#x    vbr  %#x\n\n",kdb_usp,kdb_vbr);
	printf("supervisor segtab ptr  %#x\n",kdb_mregs[0]);
	printf("   user    segtab ptr  %#x\n\n",kdb_mregs[1]);
	printf("saved kernel exception vectors\n\n");
	printf("bus        %#x\n",kdb_saved_bus);
	printf("address    %#x\n",kdb_saved_addr);
	printf("trap 14    %#x\n",kdb_saved_trap14);
	printf("trap 13    %#x\n",kdb_saved_trap15);
	printf("trace      %#x\n",kdb_saved_trace);
#endif /* CDBKDB */
}
